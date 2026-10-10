// The light, the air and the camera of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghIntro.h"
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
 * without it the engine's atmosphere) and the sky light that captures it (for Lumen), a low fog with volumetric fog in
 * which the sunlight falls into the cave in shafts, a mist over the water in a storm, a fixed exposure for each mood
 * with a film look; the figures standing out (UghFigureLook: a light on them alone, a halo around them); the wind in
 * the scanned plants (the Electric Dreams sample's foliage sways harder and with the wind in a storm); and the camera:
 * fixed, a narrow lens, a little from above, the whole screen of the original in view whatever the window's aspect and
 * the stone's edges around it (Play), flying there at the start of a level (FUghIntro). The game mode makes it the view
 * target.
 */
UCLASS()
class AUghStage : public AActor
{
	GENERATED_BODY()

public:
	AUghStage();

	/**
	 * The game's camera: the whole screen in view at `Aspect` (width / height), or only `Pixels` of it (a close-up of
	 * FUghShot). Fixed, a narrow lens, a little from above.
	 */
	static FUghCameraPose Fit(const FBox2D& Pixels, double Aspect);
	/**
	 * The camera of the play: the whole screen and around it the stone it is carved into (AUghSeaStack: at least
	 * StoneAbove pixels above the screen - the HUD lies on the stone there -, StoneBelow below it and StoneBeside beside
	 * it: the stone's face standing out in front of the level, the level carved into it), as Fit.
	 */
	static FUghCameraPose Play(double Aspect);
	/** Pixels of the stone seen around the screen at least: above it, below it (the sea), beside it. */
	static constexpr double StoneAbove = 64, StoneBelow = 34, StoneBeside = 120;
	/** The viewport's aspect now (width / height; 16:9 without one). */
	static double ViewportAspect();
	/** Puts the camera there (the game's, or on its way to it: FUghIntro). */
	void SetCamera(const FUghCameraPose& Pose);
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
	/** The figures' halo drawn or not, by ugh.Halo (the quality preset). */
	void ShowHalo();
	/** The fog's light, the mood's times ugh.FogLight (the quality preset), when either changed. */
	void LightFog();

	/** The mood's fog colour and the ugh.FogLight it was last lit with (FogLit < 0: not yet). */
	FLinearColor FogColor = FLinearColor::Black;
	float FogLit = -1;

	UPROPERTY() TObjectPtr<UCameraComponent> Camera;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> FigureFill;   // the figures' alone (UghFigureLook)
	UPROPERTY() TObjectPtr<UDirectionalLightComponent> FigureRim;
	UPROPERTY() TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY() TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY() TObjectPtr<UPostProcessComponent> Look;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> SkyDome;   // none without the skies
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SkyMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Halo;   // the figures' (UghMaterials::FigureHalo)
};
