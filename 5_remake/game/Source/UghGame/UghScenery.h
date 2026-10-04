// The decorations of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghDecorations.h"
#include "UghScenery.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The decorations of the level being played (UghDecorations): the imported models of UghAssets (a palm, rocks), each
 * scaled into its box and standing on its ledge, or clay shapes when the models are not imported (a trunk with
 * a crown, a stone). Only decoration: the game does not know it.
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
	/** A model of `Meshes` (its Variant) scaled into the box of `Decoration`, standing on its foot. */
	void AddModel(const FUghDecoration& Decoration, const TArray<TObjectPtr<UStaticMesh>>& Meshes);

	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Palms;
	UPROPERTY() TArray<TObjectPtr<UStaticMesh>> Rocks;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Models;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Trunks;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Crowns;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Stones;
};
