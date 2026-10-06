// Static meshes made at run time.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UMaterialInterface;
class UStaticMesh;
struct FMeshDescription;

/** Static meshes the frontend builds from its own vertices (one material slot, no collision). */
namespace UghMeshes
{
	/** A vertex: where it is (world), which way it faces, its tangent (the first UV's u), two UVs. */
	struct FVertex
	{
		FVector Position;
		FVector3f Normal;
		FVector3f Tangent;
		FVector2f UV0, UV1;
	};

	/**
	 * A mesh of `Vertices` and `Triangles` (three indices into them each): every triangle is turned to face the way of
	 * its vertices' normals (the engine draws the side whose corners turn clockwise).
	 */
	UStaticMesh* Build(UObject* Outer, const TArray<FVertex>& Vertices, const TArray<int32>& Triangles);
	/**
	 * A mesh of `Description` with one material slot `Slot`, built at run time (fast, no collision); the slot's UV density
	 * is set as the editor's build would (a package's texture streaming wants it).
	 */
	UStaticMesh* FromDescription(UObject* Outer, FMeshDescription& Description, FName Slot);

	/** A quad standing upright, facing the camera: its middle, size, its own numbers (the second UV). */
	struct FQuad
	{
		FVector Middle;
		FVector2D Size;
		FVector2D Seed;
	};
	/** A mesh of `Quads` (UV: u across, v down; the second UV their Seed). */
	UStaticMesh* Quads(UObject* Outer, const TArray<FQuad>& Quads);

	/** A new component of `Owner` drawing `Material`, movable, no collision, no shadow. */
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
