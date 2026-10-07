#include "UghKnockPilot.h"

#include "UghShapes.h"
#include "UghSimulation.h"

namespace
{
	/** It looks this many steps ahead at its speed (up and down), goes across at most this fast (pixels a step). */
	constexpr double Lead = 12, MaxSpeed = 1, SpeedPerPixel = 0.04, SpeedSlack = 0.05;
	/** A passenger's middle is about this far right of its sprite's corner (pixels). */
	constexpr double PassengerMiddle = 6;
	/** Its body's middle is this far right of its corner. */
	constexpr double BodyMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;

	/** A passenger on land out of its door (waiting, calling, walking): its sprite's name is "kind1.walkLeft" .. */
	bool IsOnLand(const ugh_logic* Logic, const ugh_logic_entity& Entity)
	{
		ugh_logic_sprite Info;
		if (Entity.kind != UGH_LOGIC_ENTITY_PASSENGER || Entity.sprite < 0 ||
			!ugh_logic_get_sprite(Logic, Entity.sprite, &Info))
		{
			return false;
		}
		const FString Name = UTF8_TO_TCHAR(Info.name);
		return Name.StartsWith(TEXT("kind")) && !Name.Contains(TEXT("-water")) && !Name.Contains(TEXT("comingOut")) &&
			!Name.Contains(TEXT("goingIn"));
	}

	/** The surface of the pad under a passenger at `X`, `Y` (pixels), none without one. */
	TOptional<int32> PadUnder(const ugh_logic* Logic, double X, double Y)
	{
		TOptional<int32> Surface;
		for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
		{
			ugh_logic_pad Pad;
			if (ugh_logic_get_pad(Logic, Index, &Pad) && Pad.left - 8 <= X && X <= Pad.right + 8 && Pad.y > Y &&
				(!Surface || Pad.y < *Surface))
			{
				Surface = Pad.y;
			}
		}
		return Surface;
	}
}

FUghPilotKeys FUghKnockPilot::Fly(const ugh_logic* Logic, const ugh_logic_view& Previous,
	const ugh_logic_view& Current, bool bKnocked)
{
	FUghPilotKeys Keys;
	if (!Logic || Current.copter_count == 0 || Previous.copter_count == 0)
	{
		return Keys;
	}
	const ugh_logic_copter& Copter = Current.copters[0];
	const FVector2D At = FVector2D(Copter.x, Copter.y) / UghShapes::Subpixels;
	const FVector2D Speed = FVector2D(Copter.x - Previous.copters[0].x, Copter.y - Previous.copters[0].y) /
		UghShapes::Subpixels;
	FVector2D Target = At;   // hovering where it is
	if (!bKnocked)
	{
		for (int32 I = 0; I < Current.entity_count; ++I)
		{
			const ugh_logic_entity& Entity = Current.entities[I];
			const FVector2D Corner = FVector2D(Entity.x, Entity.y) / UghShapes::Subpixels;
			const TOptional<int32> Pad = IsOnLand(Logic, Entity)
				? PadUnder(Logic, Corner.X + PassengerMiddle, Corner.Y) : TOptional<int32>();
			if (Pad)
			{
				Target.Y = *Pad - Clearance - UghShapes::CopterBodyHeight;
				if (FMath::Abs(At.Y - Target.Y) < Level)
				{
					Target.X = Corner.X + PassengerMiddle - BodyMiddle;
				}
				break;
			}
		}
	}
	Keys.bUp = At.Y + Speed.Y * Lead > Target.Y;
	const double Wanted = FMath::Clamp((Target.X - At.X) * SpeedPerPixel, -MaxSpeed, MaxSpeed);
	Keys.bRight = Speed.X < Wanted - SpeedSlack;
	Keys.bLeft = Speed.X > Wanted + SpeedSlack;
	return Keys;
}

namespace
{
	/** The solid pixels of the screen counted up and left of each (a summed-area table): a box's in four looks. */
	struct FSolids
	{
		static constexpr int32 W = UghShapes::ScreenWidth + 1, H = UghShapes::ScreenHeight + 1;
		TArray<int32> Sums;

		explicit FSolids(const ugh_logic* Logic)
		{
			Sums.Init(0, W * H);
			for (int32 Row = 1; Row < H; ++Row)
			{
				for (int32 Column = 1; Column < W; ++Column)
				{
					Sums[Row * W + Column] = (ugh_logic_solid(Logic, Column - 1, Row - 1) ? 1 : 0) +
						Sums[(Row - 1) * W + Column] + Sums[Row * W + Column - 1] - Sums[(Row - 1) * W + Column - 1];
				}
			}
		}

		/** Nothing solid in the columns `Left` .. `Right` and rows `Top` .. `Bottom` (pixels; outside the screen nothing). */
		bool IsFree(int32 Left, int32 Right, int32 Top, int32 Bottom) const
		{
			Left = FMath::Max(Left, 0);
			Right = FMath::Min(Right, W - 2) + 1;
			Top = FMath::Max(Top, 0);
			Bottom = FMath::Min(Bottom, H - 2) + 1;
			return Left >= Right || Top >= Bottom ||
				Sums[Bottom * W + Right] - Sums[Top * W + Right] - Sums[Bottom * W + Left] + Sums[Top * W + Left] == 0;
		}
	};

	/** The copter's outline across (its corner's columns + these) and down (rows), pixels (the logic's CopterShape). */
	constexpr int32 OutlineLeft = 5, OutlineRight = 25, OutlineBottom = 19;
	/** The waterline below its top (CopterShape::WATERLINE); gravity, pixels a step a step (the logic's GRAVITY). */
	constexpr int32 Waterline = 18;
	constexpr double Gravity = 27.0 / 64 / 32;
	/** It holds its height within this many pixels while it goes across. */
	constexpr double Level = 4;
}

void FUghDunkPilot::Plan(const ugh_logic* Logic, const ugh_logic_view& View)
{
	bPlanned = true;
	const FIntPoint At(FMath::FloorToInt32(double(View.copters[0].x) / UghShapes::Subpixels),
		FMath::FloorToInt32(double(View.copters[0].y) / UghShapes::Subpixels));
	const int32 Water = View.water_level / UghShapes::Subpixels;
	const int32 Bottom = Water + Below;
	const FSolids Solids(Logic);
	// the highest place to let go (the biggest splash), the nearest of those
	int32 Best = TNumericLimits<int32>::Max();
	for (int32 X = -16; X <= 304; ++X)
	{
		const int32 Left = X + OutlineLeft - Margin, Right = X + OutlineRight + Margin;
		for (int32 Top = 2; Water - Top - Waterline >= LeastFall; ++Top)
		{
			// clear from there down to under the water, the way across at that height and up or down to it
			if (Solids.IsFree(Left, Right, Top, Bottom) &&
				Solids.IsFree(FMath::Min(X, At.X) + OutlineLeft - 1, FMath::Max(X, At.X) + OutlineRight + 1,
					Top - int32(Level), Top + OutlineBottom + int32(Level)) &&
				Solids.IsFree(At.X + OutlineLeft, At.X + OutlineRight, FMath::Min(Top, At.Y) - int32(Level),
					FMath::Max(Top, At.Y) + OutlineBottom))
			{
				const int32 Score = Top * 1000 + FMath::Abs(X - At.X);
				if (Score < Best)
				{
					Best = Score;
					Place = FIntPoint(X, Top);
				}
				break;
			}
		}
	}
}

FUghPilotKeys FUghDunkPilot::Fly(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	FUghPilotKeys Keys;
	if (!Logic || Current.copter_count == 0 || Previous.copter_count == 0)
	{
		return Keys;
	}
	if (!bPlanned)
	{
		Plan(Logic, Current);
	}
	if (!Place)
	{
		return Keys;
	}
	const ugh_logic_copter& Copter = Current.copters[0];
	const FVector2D At = FVector2D(Copter.x, Copter.y) / UghShapes::Subpixels;
	const FVector2D Speed = FVector2D(Copter.x - Previous.copters[0].x, Copter.y - Previous.copters[0].y) /
		UghShapes::Subpixels;
	const FVector2D Target(Place->X, Place->Y);
	if (FMath::Abs(At.X - Target.X) < Near && FMath::Abs(At.Y - Target.Y) < Level)
	{
		bDropped = true;   // let go: it falls
	}
	if (bDropped && At.Y + Waterline >= Current.water_level / double(UghShapes::Subpixels))
	{
		return Keys;   // in the water: nothing more
	}
	// falling it still steers to stay above the place (the keys across do not hold it up)
	Keys.bUp = !bDropped && At.Y + Speed.Y * Lead > Target.Y;
	const double Across = bDropped || FMath::Abs(At.Y - Target.Y) < Level ? Target.X : At.X;
	const double Wanted = FMath::Clamp((Across - At.X) * SpeedPerPixel, -MaxSpeed, MaxSpeed);
	Keys.bRight = Speed.X < Wanted - SpeedSlack;
	Keys.bLeft = Speed.X > Wanted + SpeedSlack;
	return Keys;
}

TOptional<double> FUghDunkPilot::UntilSplash(const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	if (Current.copter_count == 0 || Previous.copter_count == 0)
	{
		return {};
	}
	const double Speed = double(Current.copters[0].y - Previous.copters[0].y) / UghShapes::Subpixels;
	const double Left = double(Current.water_level - Current.copters[0].y) / UghShapes::Subpixels - Waterline;
	if (Speed <= 0 || Left < 0)
	{
		return {};
	}
	// v n + g n^2 / 2 = Left
	const double Steps = (-Speed + FMath::Sqrt(Speed * Speed + 2 * Gravity * Left)) / Gravity;
	return Steps / FUghSimulation::TickRate;
}

namespace
{
	/** The name of an entity's sprite ("standingPassenger", "tree.swaying" ...), empty without one. */
	FString SpriteName(const ugh_logic* Logic, const ugh_logic_entity& Entity)
	{
		ugh_logic_sprite Info;
		return Entity.sprite >= 0 && ugh_logic_get_sprite(Logic, Entity.sprite, &Info) ? UTF8_TO_TCHAR(Info.name)
			: FString();
	}

	/** The stone's middle is this far right of its sprite's corner (16 px wide); the drop's hit point this far. */
	constexpr double StoneMiddle = 8, HitX = 21;
	/** The enemies are hit in a box this wide from their corner (the logic's one box for every enemy). */
	constexpr double EnemyWidth = 38;
}

FUghDropKeys FUghDropPilot::Fly(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	FUghDropKeys Keys;
	if (!Logic || Current.copter_count == 0 || Previous.copter_count == 0 || bLost)
	{
		return Keys;
	}
	const ugh_logic_copter& Copter = Current.copters[0];
	const FVector2D At = FVector2D(Copter.x, Copter.y) / UghShapes::Subpixels;
	const FVector2D Speed = FVector2D(Copter.x - Previous.copters[0].x, Copter.y - Previous.copters[0].y) /
		UghShapes::Subpixels;
	const bool bHangs = Copter.destination == -1;
	if (Stage == EStage::ToStone && bHangs)
	{
		Stage = EStage::ToEnemy;   // it took the stone: on to the enemy
		Path.Reset();
	}
	if (Stage == EStage::ToEnemy && bFired && !bHangs)
	{
		Stage = EStage::Dropped;
		bDropped = true;
	}
	FVector2D Target = At;   // hovering where it is
	if (Stage != EStage::Dropped)
	{
		if (Path.IsEmpty())
		{
			TOptional<FIntPoint> Goal;
			for (int32 I = 0; I < Current.entity_count && !Goal; ++I)
			{
				const ugh_logic_entity& Entity = Current.entities[I];
				const FVector2D Corner = FVector2D(Entity.x, Entity.y) / UghShapes::Subpixels;
				if (Stage == EStage::ToStone && Entity.kind == UGH_LOGIC_ENTITY_PASSENGER &&
					SpriteName(Logic, Entity) == TEXT("standingPassenger"))
				{
					// right above it, the skids a little into its top
					Goal = FIntPoint(FMath::RoundToInt32(Corner.X + StoneMiddle - BodyMiddle),
						FMath::FloorToInt32(Corner.Y) - UghShapes::CopterBodyHeight + Into);
				}
				else if (Stage == EStage::ToEnemy && Entity.kind == UGH_LOGIC_ENTITY_ENEMY)
				{
					// above it, so that the stone's hit point falls into the middle of its box
					Enemy = FIntPoint(FMath::FloorToInt32(Corner.X), FMath::FloorToInt32(Corner.Y));
					Goal = FIntPoint(FMath::RoundToInt32(Corner.X + EnemyWidth / 2 - HitX),
						FMath::Max(0, Enemy->Y - Above));
				}
			}
			Path = Goal ? Way(Logic, FIntPoint(FMath::FloorToInt32(At.X), FMath::FloorToInt32(At.Y)), *Goal)
				: TArray<FIntPoint>();
			Along = 0;
			if (Path.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("UGH drop pilot: no %s or no way to it"),
					Stage == EStage::ToStone ? TEXT("stone") : TEXT("enemy"));
				bLost = true;
				return Keys;
			}
		}
		// the nearest place of the way a little further on, the target a few places ahead of it
		int32 Nearest = Along;
		for (int32 I = Along; I < FMath::Min(Along + 24, Path.Num()); ++I)
		{
			if (FVector2D::Distance(At, FVector2D(Path[I])) < FVector2D::Distance(At, FVector2D(Path[Nearest])))
			{
				Nearest = I;
			}
		}
		Along = Nearest;
		Target = FVector2D(Path[FMath::Min(Along + Ahead, Path.Num() - 1)]);
		const bool bThere = Along + Ahead >= Path.Num() - 1 && FVector2D::Distance(At, Target) < Near &&
			Speed.Size() < Still;
		if (Stage == EStage::ToEnemy && (bFired || bThere))
		{
			Keys.bFire = true;   // held until the logic took it off the sling
			bFired = true;
		}
	}
	Keys.bUp = At.Y + Speed.Y * Lead > Target.Y;
	const double Wanted = FMath::Clamp((Target.X - At.X) * SpeedPerPixel * 2, -MaxSpeed * 0.8, MaxSpeed * 0.8);
	Keys.bRight = Speed.X < Wanted - SpeedSlack;
	Keys.bLeft = Speed.X > Wanted + SpeedSlack;
	return Keys;
}

TArray<FIntPoint> FUghDropPilot::Way(const ugh_logic* Logic, const FIntPoint& From, const FIntPoint& To)
{
	constexpr int32 Left = -16, Right = 304, Top = 0, Bottom = UghShapes::ScreenHeight - OutlineBottom - 2;
	constexpr int32 Columns = Right - Left + 1, Rows = Bottom - Top + 1;
	const FSolids Solids(Logic);
	const auto Free = [&](int32 X, int32 Y)
	{
		return Solids.IsFree(X + OutlineLeft - Margin, X + OutlineRight + Margin, Y - Margin,
			Y + OutlineBottom + Margin);
	};
	const auto Index = [](int32 X, int32 Y) { return (Y - Top) * Columns + (X - Left); };
	const auto Inside = [](int32 X, int32 Y) { return X >= Left && X <= Right && Y >= Top && Y <= Bottom; };
	if (!Inside(From.X, From.Y) || !Inside(To.X, To.Y) || !Free(To.X, To.Y))
	{
		return {};
	}
	TArray<int32> Parent;
	Parent.Init(-1, Columns * Rows);
	TArray<FIntPoint> Queue = { From };
	Parent[Index(From.X, From.Y)] = Index(From.X, From.Y);
	for (int32 Head = 0; Head < Queue.Num() && Parent[Index(To.X, To.Y)] < 0; ++Head)
	{
		const FIntPoint Place = Queue[Head];
		for (int32 DY = -1; DY <= 1; ++DY)
		{
			for (int32 DX = -1; DX <= 1; ++DX)
			{
				const FIntPoint Next = Place + FIntPoint(DX, DY);
				if ((DX || DY) && Inside(Next.X, Next.Y) && Parent[Index(Next.X, Next.Y)] < 0 && Free(Next.X, Next.Y))
				{
					Parent[Index(Next.X, Next.Y)] = Index(Place.X, Place.Y);
					Queue.Add(Next);
				}
			}
		}
	}
	TArray<FIntPoint> Found;
	for (int32 At = Index(To.X, To.Y); Parent[At] >= 0 && Parent[At] != At; At = Parent[At])
	{
		Found.Insert(FIntPoint(At % Columns + Left, At / Columns + Top), 0);
	}
	return Found;
}
