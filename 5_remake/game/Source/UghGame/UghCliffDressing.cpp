#include "UghCliffDressing.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghElectricDreams.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	using EKind = FUghRockPiece::EKind;

	/** The models of each kind (in the order of FUghRockPiece::EKind). */
	const TConstArrayView<const TCHAR*> KindModels[] = { UghElectricDreams::Cliffs, UghElectricDreams::Roots };
	static_assert(UE_ARRAY_COUNT(KindModels) == int32(EKind::Root) + 1);

	/** The cliffs' scans greyed into the cliff's limestone: this tint, this much of their colour, this much moss up. */
	const FLinearColor CliffTint(0.95f, 0.93f, 0.87f);
	constexpr float CliffSaturation = 0.3f, CliffMoss = 0.7f;

	/** `Mesh` fitted to `Piece`: in its ball, turned, Out of its depth out of the wall (no nearer than its limit). */
	FTransform Fit(const FUghRockPiece& Piece, const UStaticMesh* Mesh)
	{
		const FBox Bounds = Mesh->GetBoundingBox();
		const FQuat Rotation(Piece.Rotation);
		// whichever way it is turned, its box stays in a ball of half its diagonal around its middle
		const double Scale = Piece.Radius * UghShapes::UnitsPerPixel / FMath::Max(Bounds.GetExtent().Size(), 1.0);
		const FBox Turned = Bounds.TransformBy(FTransform(Rotation, FVector::ZeroVector, FVector(Scale)));
		// the world's -Y is towards the camera: the front of the turned box is its Max.Y
		const double Deep = Turned.GetSize().Y;
		const double Front = FMath::Max(Piece.Surface - Piece.Out * Deep, Piece.Nearest());
		const FVector Middle = UghShapes::ToWorld(Piece.X, Piece.Y, Front + Deep / 2);
		return FTransform(Rotation, Middle - Turned.GetCenter(), FVector(Scale));
	}
}

UMaterialInterface* AUghCliffDressing::Greyed(const UMaterialInterface* Scanned)
{
	const TPair<const TCHAR*, const TCHAR*> Maps[] = {
		{ UghElectricDreams::ScanAlbedo, UghMaterials::BaseColorParameter },
		{ UghElectricDreams::ScanNormal, UghMaterials::NormalParameter },
		{ UghElectricDreams::ScanPacked, UghMaterials::RoughnessParameter } };
	TArray<UTexture*> Textures;
	for (const TPair<const TCHAR*, const TCHAR*>& Map : Maps)
	{
		UTexture* Texture = nullptr;
		if (!Scanned || !Scanned->GetTextureParameterValue(FHashedMaterialParameterInfo(Map.Key), Texture) || !Texture)
		{
			UE_LOG(LogTemp, Display, TEXT("UGH no %s in a cliff's material: its own colours"), Map.Key);
			return nullptr;
		}
		Textures.Add(Texture);
	}
	UMaterialInstanceDynamic* Grey = UghShapes::Material(this, UghMaterials::Scan);
	for (int32 Index = 0; Index < Textures.Num(); ++Index)
	{
		Grey->SetTextureParameterValue(Maps[Index].Value, Textures[Index]);
	}
	Grey->SetVectorParameterValue(UghMaterials::ColorParameter, CliffTint);
	Grey->SetScalarParameterValue(UghMaterials::SaturationParameter, CliffSaturation);
	Grey->SetScalarParameterValue(UghMaterials::MossParameter, CliffMoss);
	return Grey;
}

AUghCliffDressing::AUghCliffDressing()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);   // their shadows are cached
}

void AUghCliffDressing::BeginPlay()
{
	Super::BeginPlay();
	for (const TConstArrayView<const TCHAR*> Paths : KindModels)
	{
		First.Add(Meshes.Num());
		Meshes.Append(UghElectricDreams::Meshes(Paths));
	}
	First.Add(Meshes.Num());
}

UInstancedStaticMeshComponent* AUghCliffDressing::InstancesOf(UStaticMesh* Mesh)
{
	if (const TObjectPtr<UInstancedStaticMeshComponent>* Found = Instances.Find(Mesh))
	{
		return *Found;
	}
	UInstancedStaticMeshComponent* Made = NewObject<UInstancedStaticMeshComponent>(this);
	Made->SetMobility(EComponentMobility::Static);
	Made->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Made->SetStaticMesh(Mesh);
	if (Meshes.Find(Mesh) < First[int32(EKind::Root)])
	{
		// a cliff: in the limestone of the cliff
		for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
		{
			if (UMaterialInterface* Grey = Greyed(Mesh->GetMaterial(Slot)))
			{
				Made->SetMaterial(Slot, Grey);
			}
		}
	}
	Made->SetupAttachment(RootComponent);
	Made->RegisterComponent();
	AddInstanceComponent(Made);
	Instances.Add(Mesh, Made);
	return Made;
}

void AUghCliffDressing::Show(const TArray<FUghRockPiece>& Pieces)
{
	TMap<UStaticMesh*, TArray<FTransform>> Shown;
	for (const TPair<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>>& Made : Instances)
	{
		Shown.Add(Made.Key);   // none of them, unless the level has some
	}
	for (const FUghRockPiece& Piece : Pieces)
	{
		const int32 Kind = int32(Piece.Kind), Count = First[Kind + 1] - First[Kind];
		if (Count > 0)
		{
			UStaticMesh* Mesh = Meshes[First[Kind] + Piece.Variant % Count];
			Shown.FindOrAdd(Mesh).Add(Fit(Piece, Mesh));
		}
	}
	for (const TPair<UStaticMesh*, TArray<FTransform>>& Model : Shown)
	{
		UghShapes::SetShapes(InstancesOf(Model.Key), Model.Value);
	}
}
