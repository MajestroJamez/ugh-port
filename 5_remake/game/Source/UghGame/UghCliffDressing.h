// The scanned rock dressing the cliff.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghRockDressing.h"
#include "UghCliffDressing.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The pieces of UghRockDressing of the level being played: the scanned cliffs (greyed into the cliff's limestone) and
 * roots of Epic's Electric Dreams sample (UghElectricDreams), each scaled into its ball, turned, and pushed into the
 * cave's back wall until only its Out comes out of it (never nearer than the piece allows), instanced (one component a
 * mesh: Nanite). Nothing without the sample's copy: the rock's own surface is all there is then. Only decoration: the
 * game does not know it.
 */
UCLASS()
class AUghCliffDressing : public AActor
{
	GENERATED_BODY()

public:
	AUghCliffDressing();

	/** Shows `Pieces` (none: nothing). */
	void Show(const TArray<FUghRockPiece>& Pieces);

protected:
	virtual void BeginPlay() override;

private:
	UInstancedStaticMeshComponent* InstancesOf(UStaticMesh* Mesh);
	/** A cliff's material `Scanned` greyed (UghMaterials::Scan with its textures); none when it has not got them. */
	UMaterialInterface* Greyed(const UMaterialInterface* Scanned);

	/** The models of each kind of piece (FUghRockPiece::EKind): Meshes[First[kind] .. First[kind + 1] - 1]. */
	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Meshes;
	TArray<int32> First;
	UPROPERTY() TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> Instances;
};
