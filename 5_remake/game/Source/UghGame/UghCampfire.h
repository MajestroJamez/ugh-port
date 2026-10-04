// The campfires in the cave.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghDecorations.h"
#include "UghCampfire.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMesh;

/**
 * The campfires of the level being played (UghDecorations, kind Campfire): a ring of stones with logs (Kenney's
 * campfire, UghAssets::Campfire, in clay of darker colours; two clay logs without it), a flame of crossed cards
 * licking upwards (the material UghMaterials::Fire) and a flickering orange light that lights the cave (Lumen), each
 * fire its own way; in the wind the flames lean and the lights flicker more. A fire goes out when the water rises
 * over its ledge. Only decoration: the game does not know them.
 */
UCLASS()
class AUghCampfire : public AActor
{
	GENERATED_BODY()

public:
	AUghCampfire();
	virtual void Tick(float DeltaSeconds) override;

	/** Lights the campfires among `Decorations` (the others are not this actor's), leaning with the level's `Wind`. */
	void Place(const TArray<FUghDecoration>& Decorations, int32 Wind);
	/** The water surface (pixels from the top): a fire burns only while the water is below its ledge. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	/** Shows the fires that burn: their stones and logs, flames and lights. */
	void Show();

	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Hearths;   // a component a mesh of the stones and logs
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Flames;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lights;   // made as many as needed, hidden when unused
	TArray<FUghDecoration> Fires;
	TArray<bool> Burning;   // of Fires
	FBox HearthBounds;      // of the stones and logs together, their meshes' units
	float Flicker = 0;      // how much the lights flicker, a part of their brightness
	int32 Wind = 0;
	double Time = 0;
};
