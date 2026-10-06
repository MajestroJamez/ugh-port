// The soft edges of the level as automation tests of the editor (Ugh.Fringe): the plants at the stone's edges are
// off the screen of the play, a copter at each of its limits runs into them, they bend away from it, hide it in the
// overhang and leave it alone elsewhere; how they sway (Ugh.Sway): hit high, in the middle, low or in the overhang, a
// liana's bottom moves about as far as where it is hit - not further, as a lever -, a little later, and settles.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UghFringe.h"
#include "UghStage.h"

namespace
{
	using EKind = FUghFringePlant::EKind;
	using ESide = FUghFringePlant::ESide;

	/** The aspects of a window the game is played in. */
	constexpr double Aspects[] = { 16.0 / 9, 16.0 / 10 };
	/**
	 * Pixels: a copter beside the level is looked at from its top limit down to here (above the water of the lowest
	 * levels); at the top limit its body (CopterBodyLeft .. Right, from its top this far down) is hidden at least this
	 * much (a share) by plants in front of its rotor (this deep, pixels).
	 */
	constexpr double LowestSide = 160, HiddenDown = 12, Hidden = 0.7, RotorDepth = -13;

	/** Where the play camera sees a point (pixels x, y and depth units) on the plane of the play. */
	FVector2D OnPlane(const FUghCameraPose& Camera, double X, double Y, double Depth)
	{
		const FVector Eye(Camera.Location.X / UghShapes::UnitsPerPixel,
			UghShapes::ScreenHeight - Camera.Location.Z / UghShapes::UnitsPerPixel, -Camera.Location.Y);
		const FVector At(X, Y, Depth);
		const FVector Hit = Eye + (At - Eye) * (-Eye.Z / (At.Z - Eye.Z));
		return FVector2D(Hit.X, Hit.Y);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFringeTest, "Ugh.Fringe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghFringeTest::RunTest(const FString& Parameters)
{
	const TArray<FUghFringePlant> Plants = UghFringe::Plan();
	TestTrue(FString::Printf(TEXT("%d plants"), Plants.Num()), Plants.Num() > 200);
	TestTrue(TEXT("the same plants every time"), UghFringe::Plan() == Plants);
	for (int32 Index = 0; Index < Plants.Num(); ++Index)
	{
		const int32 Above = Plants[Index].Above;
		TestTrue(TEXT("a curtain's piece after the one it hangs from"), Above < Index);
	}

	// off the screen of the play, as the play's camera sees them
	for (const double Aspect : Aspects)
	{
		const FUghCameraPose Camera = AUghStage::Play(Aspect);
		int32 Over = 0;
		for (const FUghFringePlant& Plant : Plants)
		{
			const FBox2D Box = Plant.Box();
			FBox2D Seen(ForceInit);
			for (const double Depth : { Plant.Depth, Plant.Tip().Z })
			{
				Seen += OnPlane(Camera, Box.Min.X, Box.Min.Y, Depth);
				Seen += OnPlane(Camera, Box.Max.X, Box.Max.Y, Depth);
			}
			const bool bOver = Seen.Max.X > 0 && Seen.Min.X < UghShapes::ScreenWidth && Seen.Max.Y > 0 &&
				Seen.Min.Y < UghShapes::ScreenHeight;
			if (bOver && Over++ < 5)
			{
				AddInfo(FString::Printf(TEXT("a plant (kind %d) over the screen: %s"), int32(Plant.Kind), *Seen.ToString()));
			}
		}
		TestEqual(FString::Printf(TEXT("aspect %.2f: no plant over the screen"), Aspect), Over, 0);
	}

	// a copter at its limits runs into them all along the edges, and they bend away from it
	auto Pushed = [&](const FVector2D& At, ESide Side, bool& bAway)
	{
		int32 Count = 0;
		for (const FUghFringePlant& Plant : Plants)
		{
			const FVector2D Bend = UghFringe::Bend(Plant, UghFringe::Reach(At));
			if (Bend.IsNearlyZero())
			{
				continue;
			}
			++Count;
			bAway &= Plant.Side == ESide::Left ? Bend.X < 0 : Plant.Side == ESide::Right ? Bend.X > 0 : Bend.Y > 0;
			bAway &= Plant.Side == Side || Plant.Side == ESide::Top;   // (the overhang in the corners)
		}
		return Count;
	};
	int32 Missed = 0;
	bool bAway = true;
	for (double Y = UghFringe::TopEdge; Y <= LowestSide; Y += 4)
	{
		for (const ESide Side : { ESide::Left, ESide::Right })
		{
			const double X = Side == ESide::Left ? UghFringe::LeftEdge : UghFringe::RightEdge;
			if (Pushed(FVector2D(X, Y), Side, bAway) == 0 && Missed++ < 5)
			{
				AddInfo(FString::Printf(TEXT("nothing bends at %.0f, %.0f"), X, Y));
			}
		}
	}
	for (double X = UghFringe::LeftEdge + 20; X <= UghFringe::RightEdge - 20; X += 4)
	{
		if (Pushed(FVector2D(X, UghFringe::TopEdge), ESide::Top, bAway) == 0 && Missed++ < 5)
		{
			AddInfo(FString::Printf(TEXT("nothing bends over %.0f"), X));
		}
	}
	TestEqual(TEXT("a copter at its limits runs into plants"), Missed, 0);
	TestTrue(TEXT("they bend away from it (beside it outwards, above it towards the camera)"), bAway);

	// the overhang hides a copter at the top limit
	int32 Points = 0, Covered = 0;
	for (double X = 30; X <= 280; X += 50)
	{
		for (double Across = UghShapes::CopterBodyLeft; Across <= UghShapes::CopterBodyRight; Across += 2)
		{
			for (double Down = 1; Down <= HiddenDown; Down += 2)
			{
				const FVector2D Point(X + Across, UghFringe::TopEdge + Down);
				++Points;
				Covered += Plants.ContainsByPredicate([&](const FUghFringePlant& Plant)
				{
					return Plant.Side == ESide::Top && FMath::Max(Plant.Depth, Plant.Tip().Z) <
						RotorDepth * UghShapes::UnitsPerPixel && Plant.Box().IsInside(Point);
				});
			}
		}
	}
	TestTrue(FString::Printf(TEXT("a copter at the top in the overhang: %d of %d points of it hidden"), Covered, Points),
		Covered >= Hidden * Points);

	// and away from the edges nothing moves
	int32 Moved = 0;
	for (double X = 20; X <= 270; X += 25)
	{
		for (double Y = 10; Y <= 160; Y += 25)
		{
			for (const FUghFringePlant& Plant : Plants)
			{
				Moved += UghFringe::Touch(Plant, UghFringe::Reach(FVector2D(X, Y))) > 0;
			}
		}
	}
	TestEqual(TEXT("nothing bends for a copter away from the edges"), Moved, 0);
	return true;
}

namespace
{
	/**
	 * A copter flown into an edge (Ugh.Sway): from Approach pixels away at Speed pixels a second, held there
	 * Hold seconds, back as fast, then gone; the sway watched this long (seconds) in frames of SwayFrame seconds.
	 */
	constexpr double Approach = 30, Speed = 60, Hold = 0.6, Watch = 14, SwayFrame = 1.0 / 60;
	/**
	 * Pixels: the bottom of a chain moves at most MostBottom times as far as where the copter holds it (a lever would
	 * swing it further the longer it is; HardlyHeld pixels more where it is hardly held), and at most MostPixels at all;
	 * held high on a curtain (at least LongBelow pixels above its bottom) the bottom follows at least LeastBottom times
	 * as far, a little later (Later seconds at least: the bend runs down it).
	 */
	constexpr double MostBottom = 1.25, HardlyHeld = 0.5, MostPixels = 11, LongBelow = 60, LeastBottom = 0.5, Later = 0.15;

	struct FSwayCase
	{
		const TCHAR* Name;
		FVector2D Edge, Away;   // the copter's top left corner at the edge, the way back from it
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFringeSwayTest, "Ugh.Sway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghFringeSwayTest::RunTest(const FString& Parameters)
{
	const TArray<FUghFringePlant> Plants = UghFringe::Plan();
	const FSwayCase Cases[] = {
		{ TEXT("left, high"), FVector2D(UghFringe::LeftEdge, 0), FVector2D(1, 0) },
		{ TEXT("left, middle"), FVector2D(UghFringe::LeftEdge, 80), FVector2D(1, 0) },
		{ TEXT("left, low"), FVector2D(UghFringe::LeftEdge, 150), FVector2D(1, 0) },
		{ TEXT("right, middle"), FVector2D(UghFringe::RightEdge, 60), FVector2D(-1, 0) },
		{ TEXT("top"), FVector2D(100, UghFringe::TopEdge), FVector2D(0, 1) } };
	for (const FSwayCase& Case : Cases)
	{
		// the copter coming, at the edge (a bump), going, gone; `Look` at the sway after each frame
		auto Fly = [&](TFunctionRef<void(const FUghFringeSway&, double)> Look)
		{
			FUghFringeSway Sway;
			Sway.Build(Plants);
			const double Arrive = Approach / Speed;
			bool bStill = false;
			for (double Time = 0; Time < Watch; Time += SwayFrame)
			{
				TArray<FBox2D> Reaches;
				TArray<double> Bumps;
				const double Back = Time - Arrive - Hold;
				if (Back < Arrive)
				{
					const double Out = Time < Arrive ? Approach - Time * Speed : FMath::Max(Back, 0.0) * Speed;
					Reaches.Add(UghFringe::Reach(Case.Edge + Case.Away * Out));
					Bumps.Add(Time >= Arrive && Time - SwayFrame < Arrive ? Speed : 0);
				}
				bStill = !Sway.Step(Reaches, Bumps, SwayFrame);
				Look(Sway, Time);
			}
			return bStill;
		};
		// how far each chain goes where it is held and at its bottom, how low it is held
		int32 Count = 0;
		TArray<double> Hit, Bottom, HitAt, BottomAt, Lowest;
		const bool bStill = Fly([&](const FUghFringeSway& Sway, double Time)
		{
			if (Count == 0)
			{
				Count = Sway.Chains().Num();
				Hit.Init(0, Count);
				Bottom.Init(0, Count);
				Lowest.Init(-UE_BIG_NUMBER, Count);
			}
			for (int32 Chain = 0; Chain < Count; ++Chain)
			{
				const FUghFringeSway::FChain& Each = Sway.Chains()[Chain];
				for (int32 Point = 1; Point < Each.Rest.Num(); ++Point)
				{
					if (Each.Held[Point] > 0)
					{
						Hit[Chain] = FMath::Max(Hit[Chain], Each.Offset[Point].Size());
						Lowest[Chain] = FMath::Max(Lowest[Chain], Each.Rest[Point].Y);
					}
				}
				Bottom[Chain] = FMath::Max(Bottom[Chain], Each.Offset.Last().Size());
			}
		});
		// when each got half as far (again, the most known)
		HitAt.Init(-1, Count);
		BottomAt.Init(-1, Count);
		Fly([&](const FUghFringeSway& Sway, double Time)
		{
			for (int32 Chain = 0; Chain < Count; ++Chain)
			{
				const FUghFringeSway::FChain& Each = Sway.Chains()[Chain];
				double Held = 0;
				for (int32 Point = 1; Point < Each.Rest.Num(); ++Point)
				{
					Held = Each.Held[Point] > 0 ? FMath::Max(Held, Each.Offset[Point].Size()) : Held;
				}
				HitAt[Chain] = HitAt[Chain] < 0 && Hit[Chain] > 0 && Held >= Hit[Chain] / 2 ? Time : HitAt[Chain];
				BottomAt[Chain] = BottomAt[Chain] < 0 && Bottom[Chain] > 0 &&
					Each.Offset.Last().Size() >= Bottom[Chain] / 2 ? Time : BottomAt[Chain];
			}
		});
		FUghFringeSway Built;
		Built.Build(Plants);

		int32 Touched = 0, Amplified = 0, Behind = 0, Early = 0;
		double MostShare = 0, Furthest = 0;
		for (int32 Chain = 0; Chain < Count; ++Chain)
		{
			if (Hit[Chain] <= 0)
			{
				continue;
			}
			++Touched;
			const FUghFringeSway::FChain& Each = Built.Chains()[Chain];
			const double Share = Bottom[Chain] / Hit[Chain];
			MostShare = FMath::Max(MostShare, Share);
			Furthest = FMath::Max(Furthest, Bottom[Chain]);
			if (Bottom[Chain] > MostBottom * Hit[Chain] + HardlyHeld && Amplified++ < 3)
			{
				AddInfo(FString::Printf(TEXT("%s: a chain at %.0f, %.0f held %.1f px, its bottom %.1f px"), Case.Name,
					Each.Rest[0].X, Each.Rest[0].Y, Hit[Chain], Bottom[Chain]));
			}
			const bool bLong = Each.Rest.Last().Y - Lowest[Chain] >= LongBelow;
			if (bLong && Share < LeastBottom && Behind++ < 3)
			{
				AddInfo(FString::Printf(TEXT("%s: a curtain at %.0f held %.1f px, its bottom only %.1f px"), Case.Name,
					Each.Rest[0].X, Hit[Chain], Bottom[Chain]));
			}
			if (bLong && Bottom[Chain] > 0.5 && BottomAt[Chain] < HitAt[Chain] + Later && Early++ < 3)
			{
				AddInfo(FString::Printf(TEXT("%s: a curtain at %.0f: its bottom at %.2f s, where it is held at %.2f s"),
					Case.Name, Each.Rest[0].X, BottomAt[Chain], HitAt[Chain]));
			}
		}
		AddInfo(FString::Printf(TEXT("%s: %d chains touched, their bottoms at most %.2f times as far as where they are held, %.1f px at most"),
			Case.Name, Touched, MostShare, Furthest));
		TestTrue(FString::Printf(TEXT("%s: the copter pushes plants"), Case.Name), Touched > 0);
		TestEqual(FString::Printf(TEXT("%s: no bottom swung further than where it is held (a lever)"), Case.Name),
			Amplified, 0);
		TestTrue(FString::Printf(TEXT("%s: the bottoms %.1f px at most"), Case.Name, Furthest), Furthest <= MostPixels);
		TestEqual(FString::Printf(TEXT("%s: held high, the curtain below follows"), Case.Name), Behind, 0);
		TestEqual(FString::Printf(TEXT("%s: a little later (the bend runs down)"), Case.Name), Early, 0);
		TestTrue(FString::Printf(TEXT("%s: then it settles"), Case.Name), bStill);
	}
	return true;
}

#endif
