// The light, the air and the camera of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghStage.generated.h"

class UCameraComponent;
class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USkyLightComponent;

/**
 * The stage of the diorama: a low warm evening sun from the front, the sky (atmosphere and sky light, for Lumen), a
 * low fog with volumetric fog, and the camera: fixed, a narrow lens, a little from above, the whole screen of the
 * original in view whatever the window's aspect. A windy level is a storm: a dim cool sun, a dense grey fog. The
 * game mode makes it the view target.
 */
UCLASS()
class AUghStage : public AActor
{
	GENERATED_BODY()

public:
	AUghStage();

	/** Moves the camera so the whole screen is in view at the viewport's current aspect. */
	void FitCamera();
	/** The weather of a level with this wind (ugh_logic_view.wind: -1, 1 a storm, 0 calm). */
	void SetWind(int32 Wind);

private:
	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
};
