#include "UghScenery.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UghAssets.h"
#include "UghElectricDreams.h"
#include "UghShapes.h"

namespace
{
	using EKind = FUghDecoration::EKind;
	using EShape = UghShapes::EShape;

	/**
	 * How a kind of decoration is drawn: its models (the scanned ones of UghElectricDreams where they were copied, else
	 * the free ones of UghAssets), whether they cast shadows (the grass and the flowers do not: thousands of cut-out
	 * tufts in the shadow maps), whether ray tracing sees them (Lumen's light bouncing: the solid ones; the cut-out
	 * leaves would cost more than they show).
	 */
	struct FLook
	{
		EKind Kind;
		TConstArrayView<const TCHAR*> Scanned, Ids;
		bool bShadow, bRayTraced;
	};
	namespace ED = UghElectricDreams;
	const FLook Looks[] = {
		{ EKind::Grass, ED::Grasses, UghAssets::Grasses, false, false },
		{ EKind::Flower, ED::Flowers, UghAssets::Flowers, false, false },
		{ EKind::Rock, ED::Rocks, UghAssets::Rocks, true, true },
		{ EKind::Bones, {}, { &UghAssets::Bones, 1 }, true, true },
		{ EKind::Fern, ED::Ferns, { &UghAssets::Fern, 1 }, true, false },
		{ EKind::Bush, ED::Bushes, UghAssets::Bushes, true, false },
		{ EKind::Plant, ED::Plants, UghAssets::Plants, true, false },
		{ EKind::Stump, ED::Stumps, { &UghAssets::Stump, 1 }, true, true },
		{ EKind::Palm, ED::Palms, { &UghAssets::Palm, 1 }, true, false },
		{ EKind::Totem, {}, { &UghAssets::Totem, 1 }, true, true },
		{ EKind::Hut, {}, { &UghAssets::Hut, 1 }, true, true },
		{ EKind::Vine, ED::Vines, { &UghAssets::Vines, 1 }, true, false },
		{ EKind::Creeper, ED::Creepers, { &UghAssets::Vines, 1 }, true, false } };

	/** A part of a kind's clay look: a shape of a colour filling a part of its box (fractions, y down from its top). */
	struct FClayPart
	{
		EKind Kind;
		EShape Shape;
		FLinearColor Color;
		FBox2D Part;
	};
	const FLinearColor Green(0.1f, 0.3f, 0.06f), Wood(0.25f, 0.13f, 0.06f), Stone(0.3f, 0.29f, 0.27f),
		Bone(0.75f, 0.7f, 0.6f);
	const FClayPart ClayParts[] = {
		{ EKind::Rock, EShape::Sphere, Stone, FBox2D(FVector2D(0, 0), FVector2D(1, 1.3)) },   // sunk into its ledge
		{ EKind::Bones, EShape::Sphere, Bone, FBox2D(FVector2D(0, 0.4), FVector2D(1, 1.2)) },
		{ EKind::Fern, EShape::Sphere, Green, FBox2D(FVector2D(0, 0), FVector2D(1, 1.1)) },
		{ EKind::Bush, EShape::Sphere, Green, FBox2D(FVector2D(0, 0), FVector2D(1, 1.1)) },
		{ EKind::Plant, EShape::Sphere, Green, FBox2D(FVector2D(0, 0), FVector2D(1, 1.1)) },
		{ EKind::Stump, EShape::Cylinder, Wood, FBox2D(FVector2D(0.1, 0), FVector2D(0.9, 1)) },
		{ EKind::Palm, EShape::Cylinder, Wood, FBox2D(FVector2D(0.45, 0.35), FVector2D(0.55, 1)) },
		{ EKind::Palm, EShape::Sphere, Green, FBox2D(FVector2D(0, 0), FVector2D(1, 0.35)) },
		{ EKind::Totem, EShape::Cylinder, Wood, FBox2D(FVector2D(0.3, 0), FVector2D(0.7, 1)) },
		{ EKind::Hut, EShape::Cone, Wood, FBox2D(FVector2D(0, 0), FVector2D(1, 1)) },
		{ EKind::Vine, EShape::Cylinder, Green, FBox2D(FVector2D(0.4, 0), FVector2D(0.6, 1)) },
		{ EKind::Creeper, EShape::Cylinder, Green, FBox2D(FVector2D(0.4, 0), FVector2D(0.6, 1)) } };

	/**
	 * A model's foot sinks this deep into its ledge (a liana's top into its ceiling; one on the back wall hangs from
	 * where it is), pixels: no gap shows.
	 */
	constexpr double ModelSunk = 0.5;

	/** The index in Looks of `Kind`'s look; none for the campfires (AUghCampfire's). */
	int32 LookOf(EKind Kind)
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Looks); ++Index)
		{
			if (Looks[Index].Kind == Kind)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** `Mesh` scaled into the box of `Decoration` (turned, as deep as wide), standing on its foot or hanging. */
	FTransform Fit(const FUghDecoration& Decoration, const UStaticMesh* Mesh)
	{
		const FBox Bounds = Mesh->GetBoundingBox();
		const FVector Size = Bounds.GetSize();
		// turned, its bounding box reaches this far across the screen and into the depth
		const double Turn = FMath::DegreesToRadians(Decoration.Yaw);
		const double Cos = FMath::Abs(FMath::Cos(Turn)), Sin = FMath::Abs(FMath::Sin(Turn));
		const double Across = FMath::Max(Cos * Size.X + Sin * Size.Y, Sin * Size.X + Cos * Size.Y);
		const double Scale = UghShapes::UnitsPerPixel *
			FMath::Min(Decoration.Height / FMath::Max(Size.Z, 1.0), Decoration.Width / FMath::Max(Across, 1.0));
		const FQuat Rotation(FVector::UpVector, Turn);
		const bool bHangs = Decoration.Hangs();
		const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, bHangs ? Bounds.Max.Z : Bounds.Min.Z);
		const double Sunk = Decoration.Kind == EKind::Creeper ? 0 : ModelSunk;
		const double Y = Decoration.Y + (bHangs ? -Sunk : Sunk);
		const FVector Location =
			UghShapes::ToWorld(Decoration.X, Y, Decoration.Depth) - Rotation.RotateVector(Anchor * Scale);
		return FTransform(Rotation, Location, FVector(Scale));
	}

	/** The box of `Part` of the box of `Decoration`. */
	FTransform ClayBox(const FUghDecoration& Decoration, const FBox2D& Part)
	{
		const double Width = Decoration.Width * (Part.Max.X - Part.Min.X);
		return UghShapes::Box(Decoration.Left() + Decoration.Width * Part.Min.X,
			Decoration.Top() + Decoration.Height * Part.Min.Y, Width, Decoration.Height * (Part.Max.Y - Part.Min.Y),
			Decoration.Depth, Width * UghShapes::UnitsPerPixel);
	}
}

AUghScenery::AUghScenery()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);   // their shadows are cached
}

void AUghScenery::BeginPlay()
{
	Super::BeginPlay();
	for (const FLook& Look : Looks)
	{
		First.Add(Meshes.Num());
		const TArray<UStaticMesh*> Found = UghElectricDreams::Meshes(Look.Scanned);
		Meshes.Append(Found.IsEmpty() ? UghAssets::Meshes(Look.Ids) : Found);
	}
	First.Add(Meshes.Num());
	for (const FClayPart& Part : ClayParts)
	{
		UInstancedStaticMeshComponent* Shapes =
			UghShapes::AddShapes(this, Part.Shape, UghShapes::Clay(this, Part.Color));
		Clay.Add(Shapes);
	}
}

UInstancedStaticMeshComponent* AUghScenery::InstancesOf(UStaticMesh* Mesh, EKind Kind)
{
	if (const TObjectPtr<UInstancedStaticMeshComponent>* Found = Instances.Find(Mesh))
	{
		return *Found;
	}
	const FLook& Look = Looks[LookOf(Kind)];
	UInstancedStaticMeshComponent* Made = NewObject<UInstancedStaticMeshComponent>(this);
	Made->SetMobility(EComponentMobility::Static);
	Made->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Made->SetStaticMesh(Mesh);
	Made->SetCastShadow(Look.bShadow);
	Made->SetVisibleInRayTracing(Look.bRayTraced);
	Made->SetupAttachment(RootComponent);
	Made->RegisterComponent();
	AddInstanceComponent(Made);
	Instances.Add(Mesh, Made);
	return Made;
}

void AUghScenery::Show(const TArray<FUghDecoration>& Decorations)
{
	TMap<UStaticMesh*, TArray<FTransform>> Models;
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Shown : Instances)
	{
		Models.Add(Shown.Key);   // none of them, unless the level has some
	}
	TArray<TArray<FTransform>> ClayBoxes;
	ClayBoxes.SetNum(UE_ARRAY_COUNT(ClayParts));
	TMap<UStaticMesh*, EKind> Kinds;
	for (const FUghDecoration& Decoration : Decorations)
	{
		const int32 Look = LookOf(Decoration.Kind);
		if (Look == INDEX_NONE)
		{
			continue;
		}
		const int32 Count = First[Look + 1] - First[Look];
		if (Count > 0)
		{
			UStaticMesh* Mesh = Meshes[First[Look] + Decoration.Variant % Count];
			Models.FindOrAdd(Mesh).Add(Fit(Decoration, Mesh));
			Kinds.Add(Mesh, Decoration.Kind);
			continue;
		}
		for (int32 Part = 0; Part < UE_ARRAY_COUNT(ClayParts); ++Part)
		{
			if (ClayParts[Part].Kind == Decoration.Kind)
			{
				ClayBoxes[Part].Add(ClayBox(Decoration, ClayParts[Part].Part));
			}
		}
	}
	for (const TPair<UStaticMesh*, TArray<FTransform>>& Model : Models)
	{
		const EKind* Kind = Kinds.Find(Model.Key);
		UghShapes::SetShapes(Kind ? InstancesOf(Model.Key, *Kind) : Instances[Model.Key].Get(), Model.Value);
	}
	for (int32 Part = 0; Part < UE_ARRAY_COUNT(ClayParts); ++Part)
	{
		UghShapes::SetShapes(Clay[Part], ClayBoxes[Part]);
	}
}
