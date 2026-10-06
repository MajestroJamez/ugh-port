// The water of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghRockMesh.h"
#include "UghWater.generated.h"

class FUghFlings;
class FUghSprites;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture;

/** Where the water is and what stirs it: only the frontend's (the logic knows its surface, ugh_logic_view). */
namespace UghWater
{
	/**
	 * The water is a sea around the cliff: from far towards the camera (units; its cut there is seen only when the
	 * camera is under the surface) to the cave's back wall, this far beyond the screen's sides (pixels).
	 */
	constexpr double Front = -5000;
	constexpr double Reach = 2000;
	/**
	 * The open sea while the camera flies in over it (FUghIntro): this far around the middle of the screen every way
	 * (units: its edge lies at the horizon, in the fog; AUghStage) and this deep below the screen (pixels: the stone's
	 * foot, AUghSeaStack).
	 */
	constexpr double OpenSea = 400000, OpenSeaDepth = 150;
	/** At most this many things stir the water (UghMaterials::RingParameters), waterfalls pour into it (FallParameters). */
	constexpr int32 MaxRings = 6, MaxFalls = 2;

	/** The surface between two steps of the logic (Alpha 0 .. 1), pixels from the top of the screen. */
	double Surface(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha);
	/**
	 * The sea with its surface at `Surface` (pixels): a box (a cube's) from Front to the cave's back wall, Reach beyond
	 * the screen's sides; none when the surface is below the screen. `bOpenSea`: the open sea (OpenSea) instead.
	 */
	TOptional<FTransform> Box(double Surface, bool bOpenSea = false);
	/**
	 * What swims or floats at the surface `Surface` (pixels) in `View` - passengers in the water, copters on it: its
	 * place on the surface (world) and how much it stirs it (w); at most MaxRings. A flung passenger (`Flings`) stirs
	 * it where it is seen.
	 */
	TArray<FVector4> Rings(const ugh_logic_view& View, const FUghSprites& Sprites, double Surface,
		const FUghFlings* Flings = nullptr);
}

/**
 * The sea around the cliff (UghMaterials::Water, a Single Layer Water material: what is under it seen through it,
 * refracted, the surface reflecting the cliff and the sky): its surface exactly at the logic's water level between two
 * steps (the rising water rises smoothly), a swell rolling in and chop drifting with the wind, foam at the rock, rings
 * around what swims or floats and around raindrops, caustics on what lies below it, the water darker and bluer the
 * deeper it is, foam where a waterfall pours in; the open sea mirroring the sky.
 */
UCLASS()
class AUghWater : public AActor
{
	GENERATED_BODY()

public:
	AUghWater();

	/**
	 * The surface at `Surface` (pixels from the top of the screen), stirred by `Rings` (UghWater::Rings); the open sea
	 * around the stone while the camera flies in (`bOpenSea`).
	 */
	void Show(double Surface, const TArray<FVector4>& Rings, bool bOpenSea = false);
	/** The weather: rain on it with wind (-1, 1; 0 calm), the sunlight's way, how bright its caustics are. */
	void SetWeather(int32 Wind, const FVector& Sun, float Caustics);
	/**
	 * The sky of the mood (none: the engine's reflections only), as bright as the camera sees it: the open sea mirrors
	 * it.
	 */
	void SetSky(UTexture* Sky, float Seen);
	/** Where waterfalls pour into it: x, y the middle of each foot (world), w its width (units); at most MaxFalls. */
	void SetFalls(TConstArrayView<FVector4> Falls);
	/** The world's z of the surface shown. */
	double SurfaceZ() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Water;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
};
