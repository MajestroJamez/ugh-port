// The water of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghRockMesh.h"
#include "UghWater.generated.h"

class FUghSprites;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Where the water is and what stirs it: only the frontend's (the logic knows its surface, ugh_logic_view). */
namespace UghWater
{
	/**
	 * The water is a sea around the cliff: from far towards the camera (units; its cut there is seen only when the
	 * camera is under the surface) to the cave's back wall, this far beyond the screen's sides (pixels).
	 */
	constexpr double Front = -5000;
	constexpr double Reach = 2000;
	/** At most this many things stir the water (UghMaterials::RingParameters). */
	constexpr int32 MaxRings = 6;

	/** The surface between two steps of the logic (Alpha 0 .. 1), pixels from the top of the screen. */
	double Surface(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha);
	/**
	 * The sea with its surface at `Surface` (pixels): a box (a cube's) from Front to the cave's back wall, Reach beyond
	 * the screen's sides; none when the surface is below the screen.
	 */
	TOptional<FTransform> Box(double Surface);
	/**
	 * What swims or floats at the surface `Surface` (pixels) in `View` - passengers in the water, copters on it: its
	 * place on the surface (world) and how much it stirs it (w); at most MaxRings.
	 */
	TArray<FVector4> Rings(const ugh_logic_view& View, const FUghSprites& Sprites, double Surface);
}

/**
 * The sea around the cliff (UghMaterials::Water, a Single Layer Water material: what is under it seen through it,
 * refracted, the surface reflecting the cliff and the sky): its surface exactly at the logic's water level between two
 * steps (the rising water rises smoothly), a swell rolling in and chop drifting with the wind, foam at the rock, rings
 * around what swims or floats and around raindrops, caustics on what lies below it, the water darker and bluer the
 * deeper it is.
 */
UCLASS()
class AUghWater : public AActor
{
	GENERATED_BODY()

public:
	AUghWater();

	/** The surface at `Surface` (pixels from the top of the screen), stirred by `Rings` (UghWater::Rings). */
	void Show(double Surface, const TArray<FVector4>& Rings);
	/** The weather: rain on it with wind (-1, 1; 0 calm), the sunlight's way, how bright its caustics are. */
	void SetWeather(int32 Wind, const FVector& Sun, float Caustics);
	/** The world's z of the surface shown. */
	double SurfaceZ() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Water;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
};
