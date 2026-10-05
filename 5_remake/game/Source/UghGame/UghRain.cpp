#include "UghRain.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UghLedges.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/**
	 * The streaks fall through a box (pixels of the screen, around it; depths, units) between the camera and the
	 * rock's face: the drops stop at the face (in the rock where it stands in front of the plane of the play).
	 */
	constexpr double RainLeft = -60, RainRight = Width + 60, RainTop = -40, RainBottom = Height + 8;
	constexpr double RainFront = -1200, RainBack = -30;
	/** How many streaks; how fast they fall along their way (cm/s), how long and wide they are (cm). */
	constexpr int32 StreakCount = 6000;
	constexpr float StreakSpeed = 2400.f, StreakLength = 70.f, StreakWidth = 4.f;
	/** The quads of the streaks are this big: the material moves their corners away (UghRain.hlsl's CornerSize). */
	constexpr double StreakCorner = 1;

	/** Splashes on the ledges: how many a pixel of a ledge, how deep (the face's top .. the plane of the play). */
	constexpr double LedgeSplashes = 2, LedgeFront = -45, LedgeBack = 10;
	/** Splashes on the water: how many, how deep (where the rain falls in front of the cliff, into the cave). */
	constexpr int32 WaterSplashes = 1500;
	constexpr double WaterBack = 350;
	/** A splash's quad: wide and high (units). */
	constexpr double SplashWidth = 30, SplashHeight = 20;

	/** A logic's raindrop: a streak this long and wide (pixels), trailing behind its head along its way. */
	constexpr double DropLength = 5, DropWidth = 0.6;
	/** The engine's plane (a card for a raindrop): this many units across, facing up. */
	constexpr double PlaneSize = 100;

	/** A quad of a mesh standing upright, facing the camera: its middle, size, the second UV (its own numbers). */
	struct FQuad
	{
		FVector Middle;
		FVector2D Size;
		FVector2D Seed;
	};

	/** A static mesh of `Quads` (UV: u across, v down; the second UV their Seed); one material slot. */
	UStaticMesh* MakeQuads(UObject* Outer, const TArray<FQuad>& Quads)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(2);
		const FName Slot = TEXT("Quads");
		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[Group] = Slot;
		const TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		const TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
		const TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
		const FVector2f Corners[] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
		for (const FQuad& Quad : Quads)
		{
			FVertexInstanceID Instances[4];
			for (int32 Corner = 0; Corner < 4; ++Corner)
			{
				const FVector2f UV = Corners[Corner];
				const FVertexID Vertex = Description.CreateVertex();
				Positions[Vertex] = FVector3f(Quad.Middle + FVector((UV.X - 0.5) * Quad.Size.X, 0, (0.5 - UV.Y) *
					Quad.Size.Y));
				Instances[Corner] = Description.CreateVertexInstance(Vertex);
				Normals[Instances[Corner]] = FVector3f::UnitY();   // towards the camera
				Tangents[Instances[Corner]] = FVector3f::UnitX();
				UVs.Set(Instances[Corner], 0, UV);
				UVs.Set(Instances[Corner], 1, FVector2f(Quad.Seed));
			}
			Description.CreateTriangle(Group, { Instances[0], Instances[2], Instances[1] });
			Description.CreateTriangle(Group, { Instances[0], Instances[3], Instances[2] });
		}
		UStaticMesh* Mesh = NewObject<UStaticMesh>(Outer);
		Mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr, Slot));
		UStaticMesh::FBuildMeshDescriptionsParams Params;
		Params.bFastBuild = true;   // at run time
		Params.bCommitMeshDescription = false;
		Params.bMarkPackageDirty = false;
		Mesh->BuildFromMeshDescriptions({ &Description }, Params);
		return Mesh;
	}

	/** The streaks: tiny quads anywhere in the rain's box, each with its own number. */
	TArray<FQuad> StreakQuads()
	{
		FRandomStream Random(StreakCount);
		TArray<FQuad> Quads;
		for (int32 I = 0; I < StreakCount; ++I)
		{
			const double X = Random.FRandRange(RainLeft, RainRight), Y = Random.FRandRange(RainTop, RainBottom);
			const FVector Middle = UghShapes::ToWorld(X, Y, Random.FRandRange(RainFront, RainBack));
			Quads.Add({ Middle, FVector2D(StreakCorner), FVector2D(Random.FRand(), 0) });
		}
		return Quads;
	}

	/** A component drawing `Material`, no collision, no shadow. */
	template <typename TComponent>
	TComponent* NewPart(AActor* Owner, UMaterialInterface* Material)
	{
		TComponent* Component = NewObject<TComponent>(Owner);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		Component->SetMaterial(0, Material);
		Component->SetupAttachment(Owner->GetRootComponent());
		Component->RegisterComponent();
		Owner->AddInstanceComponent(Component);
		return Component;
	}
}

FVector2D UghRain::Fall(int32 Wind)
{
	return FVector2D(Wind, 1).GetSafeNormal();
}

TArray<UghRain::FSpot> UghRain::Spots(const ugh_logic* Logic, int32 Seed)
{
	FRandomStream Random(Seed);
	TArray<FSpot> Spots;
	constexpr int32 Room = 2;
	for (const FUghLedge& Ledge : UghLedges::Find(Logic, Height, Room, UghLedges::PadsToo))
	{
		for (int32 I = 0; I < FMath::RoundToInt32(Ledge.Length() * LedgeSplashes); ++I)
		{
			const double X = Random.FRandRange(Ledge.First, Ledge.Last);
			const double Depth = Random.FRandRange(LedgeFront, LedgeBack);
			Spots.Add({ UghShapes::ToWorld(X, Ledge.Y, Depth), false, Random.FRand() });
		}
	}
	for (int32 I = 0; I < WaterSplashes; ++I)
	{
		const double X = Random.FRandRange(RainLeft, RainRight);
		const double Depth = Random.FRandRange(RainFront, WaterBack);
		FVector At = UghShapes::ToWorld(X, 0, Depth);
		At.Z = 0;   // lifted to the surface by the material
		Spots.Add({ At, true, Random.FRand() });
	}
	return Spots;
}

AUghRain::AUghRain()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghRain::BeginPlay()
{
	Super::BeginPlay();
	StreakMaterial = UghShapes::Material(this, UghMaterials::Rain);
	const FVector TopLeft = UghShapes::ToWorld(RainLeft, RainTop, 0), BottomRight = UghShapes::ToWorld(RainRight,
		RainBottom, 0);
	const TPair<const TCHAR*, float> Box[] = { { UghMaterials::TopParameter, TopLeft.Z },
		{ UghMaterials::BottomParameter, BottomRight.Z }, { UghMaterials::LeftParameter, TopLeft.X },
		{ UghMaterials::RightParameter, BottomRight.X }, { UghMaterials::SpeedParameter, StreakSpeed },
		{ UghMaterials::LengthParameter, StreakLength }, { UghMaterials::WidthParameter, StreakWidth } };
	for (const TPair<const TCHAR*, float>& Parameter : Box)
	{
		StreakMaterial->SetScalarParameterValue(Parameter.Key, Parameter.Value);
	}
	Streaks = NewPart<UStaticMeshComponent>(this, StreakMaterial);
	Streaks->SetStaticMesh(MakeQuads(this, StreakQuads()));
	SplashMaterial = UghShapes::Material(this, UghMaterials::Splash);
	Splashes = NewPart<UStaticMeshComponent>(this, SplashMaterial);
	Drops = NewPart<UInstancedStaticMeshComponent>(this, UghShapes::Material(this, UghMaterials::Raindrop));
	Drops->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Build(nullptr, -1, 0);
}

void AUghRain::Build(const ugh_logic* Logic, int32 LevelId, int32 InWind)
{
	Wind = Logic ? InWind : 0;
	Streaks->SetVisibility(Wind != 0);
	Splashes->SetVisibility(Wind != 0);
	if (Wind == 0)
	{
		Splashes->SetStaticMesh(nullptr);
		UghShapes::SetShapes(Drops, {});
		return;
	}
	StreakMaterial->SetScalarParameterValue(UghMaterials::WindParameter, Wind);
	TArray<FQuad> Quads;
	for (const UghRain::FSpot& Spot : UghRain::Spots(Logic, LevelId))
	{
		Quads.Add({ Spot.At + FVector(0, 0, SplashHeight / 2), FVector2D(SplashWidth, SplashHeight),
			FVector2D(Spot.Seed, Spot.bOnWater ? 1 : 0) });
	}
	Splashes->SetStaticMesh(MakeQuads(this, Quads));
}

void AUghRain::Show(const ugh_logic_view& View, double Surface)
{
	const float WaterLevel = UghShapes::ToWorld(0, Surface, 0).Z;
	StreakMaterial->SetScalarParameterValue(UghMaterials::WaterLevelParameter, WaterLevel);
	SplashMaterial->SetScalarParameterValue(UghMaterials::WaterLevelParameter, WaterLevel);
	// each drop a card along its way facing the camera, its head where the logic has the drop
	const FVector2D Way = UghRain::Fall(View.wind);
	const FVector Along = (UghShapes::ToWorld(Way.X, Way.Y, 0) - UghShapes::ToWorld(0, 0, 0)).GetSafeNormal();
	const FQuat Turn = FRotationMatrix::MakeFromXZ(Along, FVector::YAxisVector).ToQuat();
	const FVector Scale(DropLength * UghShapes::UnitsPerPixel / PlaneSize,
		DropWidth * UghShapes::UnitsPerPixel / PlaneSize, 1);
	TArray<FTransform> Cards;
	for (int32 I = 0; Wind != 0 && View.phase == UGH_LOGIC_PHASE_PLAY && I < View.raindrop_count; ++I)
	{
		const FVector2D Middle = FVector2D(View.raindrops[I][0] + 0.5, View.raindrops[I][1] + 0.5) -
			Way * DropLength / 2;
		Cards.Add(FTransform(Turn, UghShapes::ToWorld(Middle.X, Middle.Y, 0), Scale));
	}
	UghShapes::SetShapes(Drops, Cards);
}

FVector2D AUghRain::StreakFall() const
{
	// the material's way is (Wind, 0, -1) in the world: as far along x as down
	float Along = 0;
	StreakMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(UghMaterials::WindParameter), Along);
	return FVector2D(Along, 1).GetSafeNormal();
}
