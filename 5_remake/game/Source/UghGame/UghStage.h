// The light, the air and the camera of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghShapes.h"
#include "UghStage.generated.h"

class UCameraComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UPostProcessComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMeshComponent;
struct FUghMood;

/**
 * The stage of the diorama: the light and the air of a level's mood (UghMood: a day, a golden evening, a dusk, a night,
 * a storm) - the sun (or the moon) from the front left, the sky (an HDR picture of it on a dome far around, UghAssets;
 * without it the engine's atmosphere) and the sky light that captures it (for Lumen), a low fog with volumetric fog
 * in which the sunlight falls into the cave in shafts, a mist over the water in a storm, a fixed exposure for each
 * mood with a film look; the wind in the scanned plants (the Electric Dreams sample's foliage sways harder and with
 * the wind in a storm); and the camera: fixed, a narrow lens, a little from above, the whole screen of the original in
 * view whatever the window's aspect. The game mode makes it the view target.
 */
UCLASS()
class AUghStage : public AActor
{
	GENERATED_BODY()

public:
	AUghStage();

	/**
	 * Moves the camera so the whole screen is in view at the viewport's current aspect, or only `Pixels` of it (a
	 * close-up of FUghShot).
	 */
	void FitCamera(const FBox2D& Pixels = UghShapes::Screen());
	/** The light and the air of `Mood`; the plants in a level with this wind (ugh_logic_view.wind). */
	void SetMood(const FUghMood& Mood, int32 Wind);
	/** The water's surface, pixels from the top of the screen (the mist lies on it). */
	void SetWater(double Surface);
	/** Where the sunlight goes (a unit vector, world). */
	FVector SunDirection() const;

protected:
	virtual void BeginPlay() override;

private:
	/** The plants of the Electric Dreams sample sway in the wind: harder in a storm, along `Wind` (-1, 1; 0 calm). */
	void SetFoliageWind(int32 Wind);

	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY() TObjectPtr<UPostProcessComponent> Look;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SkyDome;   // none without the skies
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkyMaterial;
};
