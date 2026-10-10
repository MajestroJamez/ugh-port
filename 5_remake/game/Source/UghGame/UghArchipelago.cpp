#include "UghArchipelago.h"

#include "Async/ParallelFor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghBackground.h"
#include "UghMaterials.h"
#include "UghRockMesh.h"
#include "UghRockNoise.h"
#include "UghSurfaceNets.h"
#include "UghTexture.h"

namespace
{
	/**
	 * A shape of stone (pixels): its half width at the sea (wobbling by Wobble), narrower towards its top by Taper of
	 * it, its foot spreading Spread a pixel under the sea, its top a dome Drop lower at its sides, leaning Lean a pixel
	 * of height along x; Salt its own noise. Its height is FUghIsles::Heights.
	 */
	struct FShape
	{
		double Radius, Taper, Spread, Drop, Lean;
		uint32 Salt;
	};
	const FShape Forms[FUghIsles::Variants] = { { 230, 0.3, -3, 90, 0.0, 3 }, { 200, 0.38, -3, 70, 0.08, 5 },
		{ 260, 0.22, -3, 110, -0.06, 7 } };
	constexpr double Wobble = 0.14, TopBumps = 30, TopRound = 50;
	/** The relief (pixels), as the level's stone's, smaller: lumps, flutes, beds of slate in flakes, blocks, a notch. */
	constexpr double Lumps = 13, Flutes = 5, Beds = 12, BedHeight = 38, FlakeLength = 80, Blocks = 5, Notch = 10,
		NotchUp = 6, NotchHeight = 7, ReliefReach = 60;
	/** How far from the surface (pixels) its openness looks (a crevice, a hollow); its top's lip reaches this far down. */
	constexpr double NearLook = 4, FarLook = 12, LipReach = 40;
	/** The finest level of detail, the next, the coarsest: every so many nodes; drawn while filling this of the screen. */
	const TArray<float> ScreenSizes = { 1.f, 0.3f, 0.12f };
	/** What the cliff's material takes from the drawing for a stone (as AUghSeaStack's) and its rock's size there. */
	const FColor StoneColor(170, 150, 125);
	constexpr int32 RockLayers = 2;
	constexpr float RockScale = 2.5f;
	/** The pole on a stone's top and its flag (units): how far into the stone, how tall, how thick; the flag's size. */
	constexpr double PoleIn = 400, PoleHeight = 1500, PoleThick = 30, FlagWidth = 520, FlagHeight = 320, FlagThick = 12;
	/** The flags' colours by EUghIsle (done, open, locked) and the poles'. */
	const FLinearColor FlagColors[] = { FLinearColor(0.10f, 0.45f, 0.06f), FLinearColor(0.85f, 0.62f, 0.03f),
		FLinearColor(0.55f, 0.04f, 0.02f) };
	const FLinearColor PoleColor(0.16f, 0.10f, 0.05f);

	/** How far outside two shapes at once (A, B: outside each; negative inside), their edge rounded `Radius`. */
	double Both(double A, double B, double Radius)
	{
		const double QA = A + Radius, QB = B + Radius;
		return FVector2D(FMath::Max(QA, 0.0), FMath::Max(QB, 0.0)).Size() + FMath::Min(FMath::Max(QA, QB), 0.0) - Radius;
	}

	/** The top of a shape at x, depth (pixels above the sea) and how far from its axis (pixels). */
	double TopAt(const FShape& Shape, double Height, double X, double Depth, double Apart)
	{
		return Height - Shape.Drop * FMath::Square(Apart / Shape.Radius) +
			TopBumps * FMath::PerlinNoise2D(FVector2D(X, Depth) * 0.01 + FVector2D(Shape.Salt * 13.7, 0));
	}
}

double FUghIsleField::Value(const FVector& P, int32 Variant)
{
	const FShape& Shape = Forms[Variant];
	const double Tall = FUghIsles::Heights[Variant] / UghShapes::UnitsPerPixel;
	const double Up = UghShapes::ScreenHeight - P.Y;   // above the sea
	const double U = P.X - Shape.Lean * FMath::Max(Up, 0.0), W = P.Z;
	const double Apart = FMath::Sqrt(U * U + W * W);
	const FVector Salted = P + FVector(Shape.Salt * 101.0);
	const double Wobbled = 1 + Wobble *
		FMath::PerlinNoise3D(FVector(P.X * 0.006, P.Y * 0.0025, P.Z * 0.006) + FVector(Shape.Salt * 3.1));
	const double Radius = Shape.Radius * Wobbled * (1 - Shape.Taper * FMath::Clamp(Up / Tall, 0.0, 1.0)) +
		Shape.Spread * FMath::Max(-Up, 0.0);
	const double Outside = Both(Apart - Radius, Up - TopAt(Shape, Tall, P.X, P.Z, Apart), TopRound);
	if (FMath::Abs(Outside) >= ReliefReach)
	{
		return -Outside;
	}
	// the relief: lumps, flutes the rain cut down it, beds of slate in flakes (out most at their feet), blocks, the
	// notch of the waves
	const double Lump =
		Lumps * (FMath::PerlinNoise3D(Salted * 0.012) + 0.45 * FMath::PerlinNoise3D(Salted * 0.03 + FVector(31)));
	const double Flute = Flutes * FMath::PerlinNoise3D(FVector(Salted.X * 0.05, Salted.Y * 0.008, Salted.Z * 0.05));
	const UghRockNoise::FBlock Bed = UghRockNoise::Slate(P.X, P.Y, P.X + P.Z, BedHeight, FlakeLength, 13 + Shape.Salt);
	const UghRockNoise::FBlock Block = UghRockNoise::Blocks(P.X + P.Z, P.Y, 44, 22, 11 + Shape.Salt);
	const double Relief = Lump + Flute + Beds * (Bed.Height - 0.5) - 0.5 * Beds * (1 - Bed.Crack) +
		Blocks * (Block.Height - 0.5) - 0.5 * Blocks * (1 - Block.Crack) -
		Notch * FMath::Exp(-FMath::Square((Up - NotchUp) / NotchHeight));
	return -Outside + Relief;
}

FVector FUghIsleField::Gradient(const FVector& P, int32 Variant)
{
	constexpr double Step = 2;
	return FVector(Value(P + FVector(Step, 0, 0), Variant) - Value(P - FVector(Step, 0, 0), Variant),
		Value(P + FVector(0, Step, 0), Variant) - Value(P - FVector(0, Step, 0), Variant),
		Value(P + FVector(0, 0, Step), Variant) - Value(P - FVector(0, 0, Step), Variant)) / (2 * Step);
}

FColor FUghIsleField::Shade(const FVector& Point, const FVector& Outward, int32 Variant)
{
	// open where the field outside keeps falling as on a flat surface, closed in a crevice; the grass hangs over its top
	const double Near = FMath::Clamp(-Value(Point + Outward * NearLook, Variant) / NearLook, 0.0, 1.0);
	const double Far = FMath::Clamp(-Value(Point + Outward * FarLook, Variant) / FarLook, 0.0, 1.0);
	const FShape& Shape = Forms[Variant];
	const double Up = UghShapes::ScreenHeight - Point.Y;
	const double U = Point.X - Shape.Lean * FMath::Max(Up, 0.0);
	const double Top = TopAt(Shape, FUghIsles::Heights[Variant] / UghShapes::UnitsPerPixel, Point.X, Point.Z,
		FMath::Sqrt(U * U + Point.Z * Point.Z));
	const double Lip = 1 - FMath::SmoothStep(0.0, LipReach, Top - Up);
	return FColor(uint8(255 * (0.5 * Near + 0.5 * Far)), 0, uint8(255 * Lip), uint8(255 * FUghRockMesh::Patches(Point)));
}

void FUghIsleField::Build(int32 Variant)
{
	Values.SetNumUninitialized(Size * Height * Size);
	ParallelFor(Size, [&](int32 K)
	{
		for (int32 J = 0; J < Height; ++J)
		{
			for (int32 I = 0; I < Size; ++I)
			{
				Values[(K * Height + J) * Size + I] = Value(Origin + FVector(I, J, K) * Cell, Variant);
			}
		}
	});
}

namespace
{
	/** The surface of `Grid` (a level of detail of a shape) as a mesh. */
	template <class TGrid>
	void Surface(const TGrid& Grid, int32 Variant, FUghRockMesh& Mesh)
	{
		TArray<FVector> Points;
		TArray<FUghNetQuad> Quads;
		UghSurfaceNets::Build(Grid, Points, Quads);
		Mesh.Build(Points, Quads, [Variant](const FVector& Point)
			{
				return -FUghIsleField::Gradient(Point, Variant).GetSafeNormal();
			},
			[Variant](const FVector& Point, const FVector& Outward) { return FUghIsleField::Shade(Point, Outward, Variant); });
	}
}

AUghArchipelago::AUghArchipelago()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	SetActorHiddenInGame(true);
}

void AUghArchipelago::Make()
{
	if (!Shapes.IsEmpty())
	{
		return;
	}
	const double Started = FPlatformTime::Seconds();
	StoneMaterial = AUghBackground::MakeCliffMaterial(this);
	bCliff = StoneMaterial != nullptr;
	if (!bCliff)
	{
		StoneMaterial = UghShapes::Material(this, UghMaterials::Rock);
	}
	for (int32 Layer = 0; Layer < RockLayers; ++Layer)
	{
		const FName Size(FString(UghMaterials::CliffLayers[Layer]) + UghMaterials::SizeParameter);
		float Metres = 0;
		if (bCliff && StoneMaterial->GetScalarParameterValue(Size, Metres))
		{
			StoneMaterial->SetScalarParameterValue(Size, Metres * RockScale);
		}
	}
	StoneArt = UghTexture::Create(this, 1, 1, { StoneColor }, false);
	StoneMaterial->SetTextureParameterValue(UghMaterials::ArtParameter, StoneArt);

	Triangles.Reset();
	for (int32 Variant = 0; Variant < FUghIsles::Variants; ++Variant)
	{
		FUghIsleField Field;
		Field.Build(Variant);
		FUghRockMesh Lods[3];
		Surface(TUghIsleGrid<1>{ Field }, Variant, Lods[0]);
		Surface(TUghIsleGrid<2>{ Field }, Variant, Lods[1]);
		Surface(TUghIsleGrid<4>{ Field }, Variant, Lods[2]);
		for (const FUghRockMesh& Lod : Lods)
		{
			Triangles.Add(Lod.Triangles.Num() / 3);
		}
		Shapes.Add(FUghRockMesh::ToStaticMesh(this, { &Lods[0], &Lods[1], &Lods[2] }, ScreenSizes));

		UHierarchicalInstancedStaticMeshComponent* Stone = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		Stone->SetMobility(EComponentMobility::Static);   // its shadows are cached
		Stone->SetStaticMesh(Shapes.Last());
		Stone->SetMaterial(0, StoneMaterial);
		Stone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Stone->SetupAttachment(RootComponent);
		Stone->RegisterComponent();
		AddInstanceComponent(Stone);
		Stones.Add(Stone);
	}
	// (registered now, hidden: their pipelines are ready before they show)
	for (const FLinearColor& Color : FlagColors)
	{
		UInstancedStaticMeshComponent* Flag =
			UghShapes::AddShapes(this, UghShapes::EShape::Cube, UghShapes::Clay(this, Color));
		Flag->SetCastShadow(false);
		Flags.Add(Flag);
	}
	Poles = UghShapes::AddShapes(this, UghShapes::EShape::Cylinder, UghShapes::Clay(this, PoleColor));
	Poles->SetCastShadow(false);
	UE_LOG(LogTemp, Display, TEXT("UGH archipelago: %d shapes of %s triangles in %.0f ms"), Shapes.Num(),
		*FString::JoinBy(Triangles, TEXT("/"), [](int32 Count) { return FString::FromInt(Count); }),
		(FPlatformTime::Seconds() - Started) * 1000);
}

void AUghArchipelago::Show(const FUghIsles* Isles)
{
	if (!Isles || !Isles->IsOpen() || Isles->GetCount() == 0)
	{
		SetShown(false);
		return;
	}
	TArray<EUghIsle> States;
	for (int32 Level = 0; Level < Isles->GetCount(); ++Level)
	{
		States.Add(Isles->GetState(Level));
	}
	ShowPlaces(Isles->GetPlaces(), States, Isles->GetPlayers(), FVector::ZeroVector, INDEX_NONE);
}

void AUghArchipelago::ShowPlaces(const TArray<FUghIslePlace>& Places, const TArray<EUghIsle>& States, int32 Players,
	const FVector& Offset, int32 Hidden)
{
	if (Places.IsEmpty() || States.Num() != Places.Num())
	{
		SetShown(false);
		return;
	}
	Make();
	if (Players != ShownPlayers || States != ShownStates || Offset != ShownOffset || Hidden != ShownHidden ||
		Places.Num() != ShownCount)
	{
		ShownPlayers = Players;
		ShownStates = States;
		ShownOffset = Offset;
		ShownHidden = Hidden;
		ShownCount = Places.Num();
		TArray<FTransform> Placed[FUghIsles::Variants], Flagged[UE_ARRAY_COUNT(FlagColors)], Poled;
		for (int32 Level = 0; Level < Places.Num(); ++Level)
		{
			if (Level == Hidden)
			{
				continue;
			}
			FUghIslePlace Place = Places[Level];
			Place.Foot += Offset;
			Placed[Place.Variant].Add(FTransform(FRotator(0, Place.Yaw, 0), Place.Foot, FVector(Place.Scale)));
			// the pole on its top, the flag to its right facing the camera's side
			const FVector Foot = FUghIsles::Top(Place) - FVector(0, 0, PoleIn);
			const double Size = UghShapes::ShapeSize;
			Poled.Add(FTransform(FQuat::Identity, Foot + FVector(0, 0, PoleHeight / 2),
				FVector(PoleThick / Size, PoleThick / Size, PoleHeight / Size)));
			Flagged[int32(States[Level])].Add(FTransform(FQuat::Identity,
				Foot + FVector(FlagWidth / 2 + PoleThick / 2, 0, PoleHeight - FlagHeight / 2 - 20),
				FVector(FlagWidth / Size, FlagThick / Size, FlagHeight / Size)));
		}
		for (int32 Variant = 0; Variant < Stones.Num(); ++Variant)
		{
			Stones[Variant]->ClearInstances();
			Stones[Variant]->AddInstances(Placed[Variant], false, true, false);
			Stones[Variant]->BuildTreeIfOutdated(false, true);
		}
		for (int32 State = 0; State < Flags.Num(); ++State)
		{
			UghShapes::SetShapes(Flags[State], Flagged[State]);
		}
		UghShapes::SetShapes(Poles, Poled);
	}
	SetShown(true);
}

void AUghArchipelago::SetShown(bool bShow)
{
	if (bShow != bShown)
	{
		bShown = bShow;
		SetActorHiddenInGame(!bShow);
	}
}

void AUghArchipelago::SetWater(double Surface)
{
	if (bCliff && bShown)
	{
		StoneMaterial->SetScalarParameterValue(UghMaterials::WaterLevelParameter, UghShapes::ToWorld(0, Surface, 0).Z);
	}
}
