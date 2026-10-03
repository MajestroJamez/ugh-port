// The level around the figures.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghBackground.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class UTexture2D;
class FUghRockMesh;

/**
 * The diorama of the level being played: the rock cut in the plane of the play (FUghRockMesh, coloured by the
 * original's drawing of the level), the water, and the wooden box around the screen.
 */
UCLASS()
class AUghBackground : public AActor
{
	GENERATED_BODY()

public:
	AUghBackground();

	/** Shows the rock of a level (empty: none) coloured by `Art`, the level's drawing (FUghLevelArt). */
	void Build(const FUghRockMesh& Mesh, UTexture2D* Art);
	/** The water surface, pixels from the top of the screen. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UProceduralMeshComponent> Rock;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> RockMaterial;
	UPROPERTY() TObjectPtr<UTexture2D> RockArt;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Frame;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Water;
};
