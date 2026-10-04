// The decorations of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghDecorations.h"
#include "UghScenery.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * The decorations of the level being played (UghDecorations; the campfires are AUghCampfire's): the imported models of
 * UghAssets, each scaled into its box and standing on its ground (a liana hanging from its ceiling), instanced (one
 * component a mesh: thousands of tufts of grass are cheap for Nanite); the small and the cut-out ones neither cast
 * shadows nor are ray traced where that costs more than it shows. Clay shapes where a kind's models are not
 * imported (a trunk with a crown, a stone, a bush, a pole, a cone). Only decoration: the game does not know it.
 */
UCLASS()
class AUghScenery : public AActor
{
	GENERATED_BODY()

public:
	AUghScenery();

	/** Shows `Decorations` (none: nothing). */
	void Show(const TArray<FUghDecoration>& Decorations);

protected:
	virtual void BeginPlay() override;

private:
	/** The instances of `Mesh` (made the first time, drawn as `Kind` wants). */
	UInstancedStaticMeshComponent* InstancesOf(UStaticMesh* Mesh, FUghDecoration::EKind Kind);

	/** The models of each kind (UghScenery.cpp's Looks): Meshes[First[kind] .. First[kind + 1] - 1]. */
	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Meshes;
	TArray<int32> First;
	UPROPERTY() TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UInstancedStaticMeshComponent>> Instances;
	/** The clay shapes: a component a part of a kind's clay look (UghScenery.cpp's ClayParts). */
	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Clay;
};
