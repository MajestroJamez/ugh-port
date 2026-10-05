// The rain of a windy level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghRain.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/** Where the rain falls and splashes: only the frontend's (the logic knows its raindrops, ugh_logic_view). */
namespace UghRain
{
	/** The way the rain falls on the screen (pixels, y down) with `Wind` (-1, 1): as far with the wind as down. */
	FVector2D Fall(int32 Wind);

	/** A spot where raindrops splash: on a ledge, or on the water (its height then the water's). */
	struct FSpot
	{
		FVector At;   // the world; on the water its z is 0
		bool bOnWater;
		float Seed;   // 0 .. 1: when its drops hit
	};
	/**
	 * The spots of the level being played (its collision mask; the same for the same `Seed`): along the tops of its
	 * ledges in front of the plane of the play and on it, and over the water from its cut to deep in the cave.
	 */
	TArray<FSpot> Spots(const ugh_logic* Logic, int32 Seed);
}

/**
 * The rain of a windy level, which falls as the logic's raindrops do (as far with the wind as down, UghRain::Fall):
 * thousands of streaks between the camera and the cliff (a mesh of tiny quads the material UghMaterials::Rain moves:
 * the GPU animates them, nothing per frame here), splashes on the ledges and on the water (UghMaterials::Splash), and
 * the logic's own raindrops in the plane of the play as streaks (UghMaterials::Raindrop), where the original drew them.
 * The water's material rings with the raindrops itself (AUghWater). Nothing in a calm level.
 */
UCLASS()
class AUghRain : public AActor
{
	GENERATED_BODY()

public:
	AUghRain();

	/** The rain of the level of `Logic` (id `LevelId`) with `Wind` (-1, 1; 0: none). */
	void Build(const ugh_logic* Logic, int32 LevelId, int32 Wind);
	/** The logic's raindrops of `View` and the water's surface (pixels from the top of the screen). */
	void Show(const ugh_logic_view& View, double Surface);
	/** The way its streaks fall on the screen (pixels, y down) as their material moves them (UghRain.hlsl). */
	FVector2D StreakFall() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Streaks;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Splashes;   // a new mesh a level
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Drops;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> StreakMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SplashMaterial;
	int32 Wind = 0;
};
