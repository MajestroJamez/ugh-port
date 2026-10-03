// A campfire in the cave.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghCampfire.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/**
 * A campfire: two logs, a flame and a flickering orange light that lights the cave (Lumen). Only decoration: the
 * game does not know it. It stands where FUghRockMesh::FindHearth finds room, or is hidden.
 */
UCLASS()
class AUghCampfire : public AActor
{
	GENERATED_BODY()

public:
	AUghCampfire();
	virtual void Tick(float DeltaSeconds) override;

	/** Puts the fire on the ledge at pixel x, y (its surface); none: hidden. */
	void Place(const TOptional<FIntPoint>& Where);
	/** The water surface (pixels from the top): the fire burns only while the water is below its ledge. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Logs;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Flame;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;
	UPROPERTY() TObjectPtr<UPointLightComponent> Light;
	TOptional<FIntPoint> Hearth;   // where it stands, pixels
	bool bLit = true;   // shown and flickering, as spawned
	double Time = 0;
};
