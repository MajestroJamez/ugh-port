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
class USkyAtmosphereComponent;
class USkyLightComponent;
class UStaticMeshComponent;
class UTexture;

/**
 * The stage of the diorama: a warm evening sun from the front, the sky (an HDR picture of it on a dome far around,
 * UghAssets; without it the engine's atmosphere) and the sky light that captures it (for Lumen), a low fog with
 * volumetric fog, a fixed exposure, and the camera: fixed, a narrow lens, a little from above, the whole screen of
 * the original in view whatever the window's aspect. A windy level is a storm: a dim cool sun, a dark cloudy sky, a
 * dense grey fog. The game mode makes it the view target.
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
	/** The weather of a level with this wind (ugh_logic_view.wind: -1, 1 a storm, 0 calm). */
	void SetWind(int32 Wind);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SkyDome;   // none without the skies
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkyMaterial;
	UPROPERTY() TObjectPtr<UTexture> CalmSky;
	UPROPERTY() TObjectPtr<UTexture> StormSky;
};
