// The leaves the people wear.
#pragma once

#include "CoreMinimal.h"
#include "UghLeaves.generated.h"

class AActor;
class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;

/** A part of the leaves: a mesh in the space of the bone that carries it. */
USTRUCT()
struct FUghLeafPart
{
	GENERATED_BODY()

	FName Bone;
	UPROPERTY() TObjectPtr<UStaticMesh> Mesh;
};

/**
 * Big leaves instead of clothes: a loincloth of leaves round the hips, hanging from the belt to the middle of the
 * thighs - the front and the sides carried by the thighs, so that they go with the legs (and lie on the lap of one
 * sitting), the back by the pelvis -, and for a woman a band of leaves round the chest. Each leaf is a sheet wrapped
 * round the body a little away from it, flaring out downwards, the leaves overlapping; its picture a leaf of the
 * Electric Dreams sample's taro (UghElectricDreams::Leaves; else green clay). The sheets are made at run time for each
 * body from its rest pose (where its bones are) and its physics asset (how thick it is at the hips and the chest).
 */
USTRUCT()
struct FUghLeaves
{
	GENERATED_BODY()

	/**
	 * Makes the leaves of `Body` (a MetaHuman's, standing `Height` cm), with the band round the chest when `bTop`, their
	 * colour times `Tint` (each look its own: UghMetaHumans::LeafTints).
	 */
	void Make(const USkeletalMesh* Body, double Height, bool bTop, const FLinearColor& Tint);
	/** Adds them to `Body` (a component of `Owner` with Make's mesh), each part on its bone, hidden. */
	void Add(AActor* Owner, USkeletalMeshComponent* Body) const;
	/** The box of the leaves on `Bones` of `Body` (Make's) in its rest pose; empty without them. */
	FBox Bounds(const USkeletalMesh* Body, TConstArrayView<const TCHAR*> Bones) const;

	/** The component space transforms of the bones of `Skeleton` in its rest pose. */
	static TArray<FTransform> RestPose(const FReferenceSkeleton& Skeleton);

private:
	UPROPERTY() TArray<FUghLeafPart> Parts;
	UPROPERTY() TObjectPtr<UMaterialInterface> Material;
};
