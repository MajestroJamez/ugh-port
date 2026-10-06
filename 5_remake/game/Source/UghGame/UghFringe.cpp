#include "UghFringe.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"
#include "UghAssets.h"
#include "UghBetween.h"
#include "UghEffectPlayer.h"
#include "UghElectricDreams.h"
#include "UghShapes.h"
#include "UghStackField.h"

namespace
{
	using EKind = FUghFringePlant::EKind;
	using ESide = FUghFringePlant::ESide;
	using Field = FUghStackField;

	/** The stone's face around its hollow (pixels deep: Field::FrameDepth), a plant on it a little in front. */
	constexpr double Face = Field::FrameDepth - 1.5;

	/**
	 * The overhang over the level: lianas hanging from the hollow's top edge (Lip) in two rows - in front of the
	 * copter's rotor (it sweeps 13 px around its hub, in the depth too) from Front to Back pixels deep -, StrandStep
	 * pixels apart, their tips TipHigh to TipLow pixels above the screen (never over it); ferns and roots hanging among
	 * them every FernStep and RootStep pixels; bushes on the face over the lip every BushStep pixels sticking out
	 * towards the camera (the overhang's top), creepers on the face down to the lip every CreeperStep pixels.
	 */
	constexpr double Lip = Field::HoleTop, StrandStep = 3, FernStep = 8, RootStep = 24, BushStep = 13,
		CreeperStep = 28;
	constexpr double FrontRow[] = { -25, -20.5 }, BackRow[] = { -20, -15 }, TipHigh = -8, TipLow = -1.5;
	/** In front of them a veil of ivy (creepers) VeilStep pixels apart, VeilRow deep. */
	constexpr double VeilStep = 8, VeilRow[] = { -29, -25.5 };
	/** A fern's fronds spread this many times its length. */
	constexpr double FernWide = 1.1;
	/**
	 * Beside the level (the left; the right mirrored): curtains of lianas from the hollow's top edge down to the sea
	 * (Curtains: their x, pixels, nearest last - their leaves reach over the copter's side at its limit -, depth: in front
	 * of the copter or behind it, and width), each of pieces
	 * PieceShort to PieceLong pixels long overlapping by PieceOverlap; bushes and ferns sticking out of the face by the
	 * hollow's edge every SideBushStep pixels down, creepers on the face every SideCreeperStep.
	 */
	struct FCurtain
	{
		double X, Depth, Width;
		bool bLeafy;   // of wall ivy (else of lianas)
	};
	constexpr FCurtain Curtains[] = { { -41, -7, 11, false }, { -36, 6, 13, true }, { -31, -12, 10, false },
		{ -26, 5, 13, true }, { -21, -7, 10, false }, { -16.5, 7, 12, true }, { -12.5, -11, 9, false },
		{ -8.5, -8, 10, true }, { -6.5, 5, 7, false } };
	constexpr double PieceShort = 26, PieceLong = 44, PieceOverlap = 3, CurtainEnd = Field::HoleBottom - 4;
	constexpr double SideBushStep = 26, SideCreeperStep = 38;
	constexpr int32 Seed = 2404;

	/** The way a plant grows: down, or out of the face towards the camera (and a little `Aside`, `Up`). */
	FVector Out(double Aside, double Up)
	{
		return FVector(Aside, 1, Up).GetSafeNormal();   // (the world's +Y is towards the camera)
	}

	/** The scanned models of each kind (in the order of EKind); a liana of Blender's vines.py without them. */
	TConstArrayView<const TCHAR*> ScannedOf(EKind Kind)
	{
		namespace ED = UghElectricDreams;
		switch (Kind)
		{
		case EKind::Strand: return ED::Vines;
		case EKind::Fern: return ED::Ferns;
		case EKind::Root: return ED::Roots;
		case EKind::Bush: return ED::Bushes;
		default: return ED::Creepers;
		}
	}

	/**
	 * How a kind's model grows from where it is attached: along this axis of its own (a liana and a creeper hang from
	 * their top, a fern and a bush grow up from their foot, a root lies along +Y from its trunk's end).
	 */
	FVector GrowthOf(EKind Kind)
	{
		switch (Kind)
		{
		case EKind::Strand:
		case EKind::Creeper: return -FVector::UpVector;
		case EKind::Root: return FVector::YAxisVector;
		default: return FVector::UpVector;
		}
	}

	/** `Mesh` fitted to `Plant`: attached at its pivot, grown its length its way, its width wide (stretched a little). */
	FTransform Fit(const FUghFringePlant& Plant, const UStaticMesh* Mesh)
	{
		const FBox Bounds = Mesh->GetBoundingBox();
		const FVector Growth = GrowthOf(Plant.Kind);
		const FVector Along = Growth.GetAbs();
		const FVector Size = Bounds.GetSize();
		const double Length = FMath::Max(Size | Along, 1.0);
		const double Across = FMath::Max((Size * (FVector::OneVector - Along)).GetMax(), 1.0);
		const double ScaleLength = Plant.Length * UghShapes::UnitsPerPixel / Length;
		const double ScaleAcross = FMath::Clamp(Plant.Width * UghShapes::UnitsPerPixel / Across, ScaleLength / 1.6,
			ScaleLength * 1.6);
		const FVector Scale = Along * ScaleLength + (FVector::OneVector - Along) * ScaleAcross;
		// attached where it starts along its growth, in the middle across
		const FVector Anchor = Bounds.GetCenter() - Growth * (Size / 2 | Along);
		const FQuat Rotation = FQuat::FindBetweenNormals(Growth, Plant.Grows) *
			FQuat(Growth, FMath::DegreesToRadians(Plant.Twist));
		const FVector Pivot = UghShapes::ToWorld(Plant.X, Plant.Y, Plant.Depth);
		return FTransform(Rotation, Pivot - Rotation.RotateVector(Anchor * Scale), Scale);
	}

	/** The spring of a plant's bend: its own swing (a second), how much it is damped (of critical). */
	constexpr double SwingRate = 1.3, Damping = 0.28;
	/** Seconds of a step of its spring at most. */
	constexpr double SpringStep = 1.0 / 120;
	/**
	 * A copter's bump into an edge: from this speed (pixels a second) it shakes the plants (degrees a second for a
	 * pixel a second, at most), a few leaves fall (the burst Rustle, at most every LeavesEvery seconds a copter);
	 * while it is in them they shiver (degrees a second, a frame).
	 */
	constexpr double BumpSpeed = 12, BumpKick = 1.5, MostKick = 120, LeavesEvery = 0.7, Shiver = 260;
	/** Below this a plant is at rest (degrees, degrees a second). */
	constexpr double Rest = 0.02;
}

FVector FUghFringePlant::Tip() const
{
	return FVector(X + Grows.X * Length, Y - Grows.Z * Length, Depth - Grows.Y * Length * UghShapes::UnitsPerPixel);
}

FBox2D FUghFringePlant::Box() const
{
	const FVector End = Tip();
	const FVector2D Half(Width / 2, Width / 2);
	FBox2D Box(FVector2D(X, Y) - Half, FVector2D(X, Y) + Half);
	Box += FVector2D(End.X, End.Y) - Half;
	Box += FVector2D(End.X, End.Y) + Half;
	// a hanging one is as wide as it is, not taller than its length
	if (Grows.Z < -0.9)
	{
		Box.Min.Y = Y;
		Box.Max.Y = End.Y;
	}
	return Box;
}

TArray<FUghFringePlant> UghFringe::Plan()
{
	FRandomStream Random(Seed);
	TArray<FUghFringePlant> Plants;
	auto Add = [&](EKind Kind, ESide Side, double X, double Y, double Depth, const FVector& Grows, double Length,
		double Width, bool bSways, int32 Above = INDEX_NONE)
	{
		FUghFringePlant Plant;
		Plant.Kind = Kind;
		Plant.Side = Side;
		Plant.X = X;
		Plant.Y = Y;
		Plant.Depth = Depth * UghShapes::UnitsPerPixel;
		Plant.Grows = Grows.GetSafeNormal();
		Plant.Length = Length;
		Plant.Width = Width;
		Plant.Twist = Kind == EKind::Creeper ? Random.FRandRange(-25, 25) : Random.FRandRange(-180, 180);
		Plant.Variant = Random.RandHelper(1000);
		Plant.bSways = bSways;
		Plant.Above = Above;
		return Plants.Add(Plant);
	};
	const double Left = Field::HoleLeft, Right = Field::HoleRight;

	// the overhang: a veil of ivy in front, lianas in two rows, ferns and roots among them, hanging from the lip
	for (double X = Left + Random.FRand() * VeilStep; X < Right; X += VeilStep * Random.FRandRange(0.7, 1.3))
	{
		const double Y = Lip + Random.FRandRange(-3, 0);
		Add(EKind::Creeper, ESide::Top, X, Y, Random.FRandRange(VeilRow[0], VeilRow[1]), -FVector::UpVector,
			Random.FRandRange(TipHigh, TipLow) - Y, Random.FRandRange(12, 18), true);
	}
	for (const double* Row : { FrontRow, BackRow })
	{
		for (double X = Left + 1 + Random.FRand() * StrandStep; X < Right - 1; X += StrandStep * Random.FRandRange(0.7, 1.3))
		{
			const double Y = Lip + Random.FRandRange(-3, 1.5);
			const double Tip = Random.FRandRange(TipHigh, TipLow);
			Add(EKind::Strand, ESide::Top, X, Y, Random.FRandRange(Row[0], Row[1]), -FVector::UpVector, Tip - Y,
				Random.FRandRange(7, 12), true);
		}
	}
	for (double X = Left + Random.FRand() * FernStep; X < Right; X += FernStep * Random.FRandRange(0.7, 1.3))
	{
		const FVector Grows(Random.FRandRange(-0.3, 0.3), 0.55, -0.8);
		const double Y = Lip - 1;
		// its tip and its fronds' spread (half its width around its tip) above the screen
		const double Down = -Grows.GetSafeNormal().Z;
		const double Length = FMath::Min(Random.FRandRange(13, 17), (TipLow - 1 - Y) / (Down + FernWide / 2));
		Add(EKind::Fern, ESide::Top, X, Y, Random.FRandRange(-22, -15), Grows, Length, Length * FernWide, true);
	}
	for (double X = Left + 6 + Random.FRand() * RootStep; X < Right - 6; X += RootStep * Random.FRandRange(0.7, 1.3))
	{
		const double Y = Lip - 0.5;
		Add(EKind::Root, ESide::Top, X, Y, Random.FRandRange(-20, -13),
			FVector(Random.FRandRange(-0.25, 0.25), Random.FRandRange(0.1, 0.4), -1), Random.FRandRange(10, 13),
			Random.FRandRange(6, 8), true);
	}
	for (double X = Left - 8 + Random.FRand() * BushStep; X < Right + 8; X += BushStep * Random.FRandRange(0.75, 1.25))
	{
		const double Length = Random.FRandRange(16, 24);
		Add(EKind::Bush, ESide::Top, X, Lip - Random.FRandRange(4, 11), Face, Out(Random.FRandRange(-0.4, 0.4),
			Random.FRandRange(-0.25, 0.35)), Length, Length * 1.2, false);
	}
	for (double X = Left - 10 + Random.FRand() * CreeperStep; X < Right + 10; X += CreeperStep * Random.FRandRange(0.7, 1.3))
	{
		Add(EKind::Creeper, ESide::Top, X, Lip - Random.FRandRange(14, 20), Face + 0.5, -FVector::UpVector,
			Random.FRandRange(12, 17), Random.FRandRange(18, 26), false);
	}

	// beside it: the left mirrored to the right
	for (const ESide Side : { ESide::Left, ESide::Right })
	{
		auto Across = [Side](double X) { return Side == ESide::Left ? X : UghShapes::ScreenWidth - X; };
		const double Mirror = Side == ESide::Left ? 1 : -1;
		for (const FCurtain& Curtain : Curtains)
		{
			int32 Above = INDEX_NONE;
			double Y = Lip - Random.FRandRange(1, 3);
			const double X = Curtain.X + Random.FRandRange(-0.6, 0.6);
			while (Y < CurtainEnd)
			{
				const double Length = Random.FRandRange(PieceShort, PieceLong);
				Above = Add(Curtain.bLeafy ? EKind::Creeper : EKind::Strand, Side, Across(X), Y,
					Curtain.Depth + Random.FRandRange(-1, 1), -FVector::UpVector, Length,
					Curtain.Width * Random.FRandRange(0.9, 1.1), true, Above);
				Y += Length - PieceOverlap;
			}
		}
		for (double Y = Lip + Random.FRand() * SideBushStep; Y < CurtainEnd; Y += SideBushStep * Random.FRandRange(0.75, 1.25))
		{
			const double Length = Random.FRandRange(13, 20);
			const bool bFern = Random.FRand() < 0.4;
			Add(bFern ? EKind::Fern : EKind::Bush, Side, Across(Left - Random.FRandRange(4, 12)), Y, Face,
				Out(Mirror * Random.FRandRange(0.4, 0.8), Random.FRandRange(-0.3, 0.3)), Length, Length * 1.2, false);
		}
		for (double Y = Lip - 6 + Random.FRand() * SideCreeperStep; Y < CurtainEnd; Y += SideCreeperStep * Random.FRandRange(0.7, 1.3))
		{
			Add(EKind::Creeper, Side, Across(Left - Random.FRandRange(10, 20)), Y, Face + 0.5, -FVector::UpVector,
				Random.FRandRange(28, 42), Random.FRandRange(14, 20), false);
		}
	}
	return Plants;
}

FBox2D UghFringe::Reach(const FVector2D& At)
{
	return FBox2D(At + FVector2D(ReachLeft, ReachTop), At + FVector2D(ReachRight, ReachBottom));
}

double UghFringe::Touch(const FUghFringePlant& Plant, const FBox2D& Reach)
{
	if (!Plant.Bends())
	{
		return 0;
	}
	const FBox2D Box = Plant.Box();
	const double Apart = FVector2D(FMath::Max3(Reach.Min.X - Box.Max.X, Box.Min.X - Reach.Max.X, 0.0),
		FMath::Max3(Reach.Min.Y - Box.Max.Y, Box.Min.Y - Reach.Max.Y, 0.0)).Size();
	return FMath::Clamp(1 - Apart / Near, 0.0, 1.0);
}

FVector2D UghFringe::Bend(const FUghFringePlant& Plant, const FBox2D& Reach)
{
	const double Touched = Touch(Plant, Reach);
	if (Touched <= 0)
	{
		return FVector2D::ZeroVector;
	}
	const FBox2D Box = Plant.Box();
	// how far into it the copter reaches across (a share of its width) and down it (a share of its length)
	const double Into = FMath::Clamp(
		(FMath::Min(Box.Max.X, Reach.Max.X) - FMath::Max(Box.Min.X, Reach.Min.X)) / FMath::Max(Box.GetSize().X, 1.0),
		0.0, 1.0);
	const double Down = FMath::Clamp(
		(FMath::Min(Box.Max.Y, Reach.Max.Y) - FMath::Max(Box.Min.Y, Reach.Min.Y)) / FMath::Max(Box.GetSize().Y, 1.0),
		0.0, 1.0);
	// away from the copter: beside the level outwards, above it apart from its middle, and towards the camera
	const double Away = Plant.Side == ESide::Left ? -1 : Plant.Side == ESide::Right ? 1
		: Box.GetCenter().X < Reach.GetCenter().X ? -1 : 1;
	if (Plant.Side == ESide::Top)
	{
		// the overhang drapes over it: a little apart, most towards the camera (it stays hidden in it)
		return FVector2D(Away * Touched * (0.3 + 0.7 * Into) * TopAside, Touched * (0.3 + 0.7 * Down) * TopForward);
	}
	// a curtain: outwards, the pieces in front of it towards the camera, those behind it away
	const double Aside = Away * Touched * (0.35 + 0.65 * Into) * (0.5 + 0.5 * Down) * SideAside;
	const double Forward = (Plant.Depth > 0 ? -1 : 1) * Touched * (0.4 + 0.6 * Into) * SideForward;
	return FVector2D(Aside, Forward);
}

AUghFringe::AUghFringe()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	PrimaryActorTick.bCanEverTick = false;
}

void AUghFringe::BeginPlay()
{
	Super::BeginPlay();
	Plants = UghFringe::Plan();
	// the models of each kind; without the sample only the lianas, Blender's
	TMap<EKind, TArray<UStaticMesh*>> Models;
	for (const EKind Kind : { EKind::Strand, EKind::Fern, EKind::Root, EKind::Bush, EKind::Creeper })
	{
		TArray<UStaticMesh*> Found = UghElectricDreams::IsCopied() ? UghElectricDreams::Meshes(ScannedOf(Kind))
			: TArray<UStaticMesh*>();
		if (Found.IsEmpty() && Kind == EKind::Strand)
		{
			Found = UghAssets::Meshes({ &UghAssets::Vines, 1 });
		}
		Models.Add(Kind, Found);
	}
	TMap<TPair<UStaticMesh*, bool>, int32> ComponentOf;   // by its mesh and whether it bends
	TArray<TArray<FTransform>> Instances;
	Shown.SetNum(Plants.Num());
	for (int32 Index = 0; Index < Plants.Num(); ++Index)
	{
		const FUghFringePlant& Plant = Plants[Index];
		const TArray<UStaticMesh*>& Meshes = Models[Plant.Kind];
		if (Meshes.IsEmpty())
		{
			continue;
		}
		UStaticMesh* Mesh = Meshes[Plant.Variant % Meshes.Num()];
		int32* Component = ComponentOf.Find({ Mesh, Plant.Bends() });
		if (!Component)
		{
			UInstancedStaticMeshComponent* Made = NewObject<UInstancedStaticMeshComponent>(this);
			// a bending plant moves (its shadow too); the still ones are cached
			Made->SetMobility(Plant.Bends() ? EComponentMobility::Movable : EComponentMobility::Static);
			Made->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Made->SetStaticMesh(Mesh);
			Made->SetVisibleInRayTracing(false);   // leaves: Lumen's bounce would cost more than it shows
			Made->SetupAttachment(RootComponent);
			Made->RegisterComponent();
			AddInstanceComponent(Made);
			Component = &ComponentOf.Add({ Mesh, Plant.Bends() }, Components.Add(Made));
			Instances.AddDefaulted();
		}
		FShown& Each = Shown[Index];
		Each.Mesh = *Component;
		Each.Instance = Instances[*Component].Num();
		Each.Rest = Fit(Plant, Mesh);
		Each.Pivot = UghShapes::ToWorld(Plant.X, Plant.Y, Plant.Depth);
		Instances[*Component].Add(Each.Rest);
	}
	for (int32 Component = 0; Component < Components.Num(); ++Component)
	{
		Components[Component]->AddInstances(Instances[Component], false, true);
	}
	UE_LOG(LogTemp, Display, TEXT("UGH fringe: %d plants, %d models"), Plants.Num(), Components.Num());
}

void AUghFringe::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
	FUghEffectPlayer& Effects)
{
	if (Shown.IsEmpty() || Seconds <= 0)
	{
		return;
	}
	// the copters now: where they reach, a bump into an edge
	const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Current.level_id >= 0;
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	TArray<FBox2D, TInlineAllocator<2>> Reaches;
	TArray<double, TInlineAllocator<2>> Bumps;   // pixels a second into an edge, 0 none
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(Seen); ++Player)
	{
		FCopterSeen& Copter = Seen[Player];
		if (!bPlay || Player >= Current.copter_count)
		{
			Copter = FCopterSeen();
			continue;
		}
		const ugh_logic_copter& To = Current.copters[Player];
		const ugh_logic_copter& Was = Player < From.copter_count ? From.copters[Player] : To;
		const FVector2D At = UghBetween::Position(Was.x, Was.y, To.x, To.y, Alpha);
		const FVector2D Velocity = Copter.At ? (At - *Copter.At) / Seconds : FVector2D::ZeroVector;
		const bool bEdges[3] = { At.X <= UghFringe::LeftEdge + 0.01, At.X >= UghFringe::RightEdge - 0.01,
			At.Y <= UghFringe::TopEdge + 0.01 };
		const double Into[3] = { -Copter.Velocity.X, Copter.Velocity.X, -Copter.Velocity.Y };
		const FVector2D Places[3] = { FVector2D(UghFringe::LeftEdge + UghFringe::ReachLeft - 2, At.Y + 8),
			FVector2D(UghFringe::RightEdge + UghFringe::ReachRight + 2, At.Y + 8),
			FVector2D(At.X + 16, UghFringe::TopEdge + 1) };
		double Bump = 0;
		Copter.Quiet += Seconds;
		for (int32 Edge = 0; Edge < 3; ++Edge)
		{
			if (bEdges[Edge] && !Copter.bAtEdge[Edge] && Copter.At && Into[Edge] >= BumpSpeed)
			{
				Bump = FMath::Max(Bump, Into[Edge]);
				UE_LOG(LogTemp, Display, TEXT("UGH fringe: copter %d into the %s edge at %.0f px/s"), Player + 1,
					Edge == 0 ? TEXT("left") : Edge == 1 ? TEXT("right") : TEXT("top"), Into[Edge]);
				if (Copter.Quiet >= LeavesEvery)
				{
					Effects.Order(EUghBurst::Rustle, Places[Edge], Current, FMath::Clamp(Into[Edge] / 60, 0.6, 1.3));
					Copter.Quiet = 0;
				}
			}
			Copter.bAtEdge[Edge] = bEdges[Edge];
		}
		Copter.Velocity = FMath::Lerp(Copter.Velocity, Velocity, FMath::Min(Seconds * 20, 1.0));
		Copter.At = At;
		Reaches.Add(UghFringe::Reach(At));
		Bumps.Add(Bump);
	}
	if (bStill && Reaches.IsEmpty())
	{
		return;
	}

	// each plant: its spring towards where the copters push it, then where its bend takes it (and the pieces under it)
	const double Stiff = FMath::Square(UE_TWO_PI * SwingRate), Damp = 2 * Damping * UE_TWO_PI * SwingRate;
	const int32 Steps = FMath::Clamp(FMath::CeilToInt32(Seconds / SpringStep), 1, 24);
	const double Step = Seconds / Steps;
	bool bAnyMoving = false;
	for (int32 Index = 0; Index < Plants.Num(); ++Index)
	{
		const FUghFringePlant& Plant = Plants[Index];
		FShown& Each = Shown[Index];
		if (Each.Mesh == INDEX_NONE)
		{
			continue;
		}
		const bool bAboveMoved = Plant.Above != INDEX_NONE && Shown[Plant.Above].bMoved;
		FVector2D Target = FVector2D::ZeroVector;
		if (Plant.Bends())
		{
			for (int32 Copter = 0; Copter < Reaches.Num(); ++Copter)
			{
				const FVector2D Push = UghFringe::Bend(Plant, Reaches[Copter]);
				if (Push.GetAbsMax() > Target.GetAbsMax())
				{
					Target = Push;
				}
				const double Touched = UghFringe::Touch(Plant, Reaches[Copter]);
				if (Touched > 0)
				{
					// a bump shakes it away from the copter, being in it makes it shiver
					const FVector2D Away = Push.IsNearlyZero() ? FVector2D(0, 1) : Push.GetSafeNormal();
					Each.Spin += Away * FMath::Min(Bumps[Copter] * BumpKick, MostKick) * Touched;
					Each.Spin += FVector2D(FMath::FRandRange(-1.0, 1.0), FMath::FRandRange(-1.0, 1.0)) * Shiver *
						Touched * FMath::Min(Seconds, 0.05);
				}
			}
		}
		const FVector2D Before = Each.Angle;
		for (int32 Count = 0; Count < Steps; ++Count)
		{
			Each.Spin += (Stiff * (Target - Each.Angle) - Damp * Each.Spin) * Step;
			Each.Angle += Each.Spin * Step;
		}
		if (Target.IsNearlyZero() && Each.Angle.GetAbsMax() < Rest && Each.Spin.GetAbsMax() < Rest)
		{
			Each.Angle = Each.Spin = FVector2D::ZeroVector;
		}
		Each.bMoved = bAboveMoved || !(Each.Angle - Before).IsNearlyZero(1e-4);
		bAnyMoving |= !Each.Angle.IsNearlyZero() || !Each.Spin.IsNearlyZero();
		if (!Each.bMoved)
		{
			continue;
		}
		// its tip aside (to the screen's right), towards the camera (the world's +Y): turned about its pivot
		const FQuat Turn = FQuat(FVector::YAxisVector, -FMath::DegreesToRadians(Each.Angle.X)) *
			FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Each.Angle.Y));
		const FTransform Own = FTransform(-Each.Pivot) * FTransform(Turn) * FTransform(Each.Pivot);
		Each.Bent = Plant.Above != INDEX_NONE ? Own * Shown[Plant.Above].Bent : Own;
		Components[Each.Mesh]->UpdateInstanceTransform(Each.Instance, Each.Rest * Each.Bent, true, true, true);
	}
	bStill = !bAnyMoving;
}
