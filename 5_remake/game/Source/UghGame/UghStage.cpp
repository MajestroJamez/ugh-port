#include "UghStage.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UghShapes.h"

namespace
{
	/** The sun from the front above and a little from the left (degrees). */
	const FRotator SunDirection(-30, -70, 0);
	/** A fog low over the water (the volumetric fog lets the campfire's light glow): how fast it thins upwards. */
	constexpr float FogFalloff = 0.3f;

	/** The light and the air of a weather: the sun (colour, lux), the sky light, the fog (density, colour). */
	struct FWeather
	{
		FLinearColor SunColor;
		float SunLux, SkyIntensity, FogDensity;
		FLinearColor FogColor;
	};
	/** Calm: a low warm evening sun, a thin fog of the engine's colour. */
	const FWeather Calm{ FLinearColor(1.f, 0.88f, 0.72f), 4.f, 1.2f, 0.01f, FLinearColor(0.447f, 0.638f, 1.f) };
	/** The wind of a level brings a storm: a dim cool sun, a dense grey fog. */
	const FWeather Storm{ FLinearColor(0.7f, 0.8f, 1.f), 1.2f, 0.7f, 0.05f, FLinearColor(0.3f, 0.33f, 0.38f) };

	/** The camera's horizontal field of view and how much it looks down (degrees); room around the screen. */
	constexpr float FieldOfView = 30.f, LookDown = 4.f;
	constexpr double ScreenMargin = 1.08;

	/** Darker than the eye would choose: an evening, the campfire stands out. */
	constexpr float ExposureBias = -1.f;
}

AUghStage::AUghStage()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(SunDirection);
	Sun->SetAtmosphereSunLight(false);   // the atmosphere would tint it orange at this low angle: the cave keeps its colours

	CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Sky"))->SetupAttachment(RootComponent);
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->SetFogHeightFalloff(FogFalloff);
	Fog->SetVolumetricFog(true);

	UPostProcessComponent* Look = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Look"));
	Look->SetupAttachment(RootComponent);
	Look->bUnbound = true;
	Look->Settings.bOverride_AutoExposureBias = true;
	Look->Settings.AutoExposureBias = ExposureBias;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(FieldOfView);
	Camera->SetConstraintAspectRatio(false);
	Camera->SetRelativeRotation(FRotator(-LookDown, -90, 0));   // looking along -Y (UghShapes)
	SetWind(0);
}

void AUghStage::FitCamera()
{
	FVector2D Viewport(16, 9);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(Viewport);
	}
	const double Aspect = Viewport.Y > 0 ? Viewport.X / Viewport.Y : 16.0 / 9.0;
	const double HalfTan = FMath::Tan(FMath::DegreesToRadians(FieldOfView / 2));
	const double Width = UghShapes::ScreenWidth * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Height = UghShapes::ScreenHeight * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Distance = FMath::Max(Width / 2 / HalfTan, Height / 2 * Aspect / HalfTan);
	// looking down at the middle of the screen from above it
	const double Above = Distance * FMath::Tan(FMath::DegreesToRadians(LookDown)) / UghShapes::UnitsPerPixel;
	Camera->SetWorldLocation(UghShapes::ToWorld(UghShapes::ScreenWidth / 2.0, UghShapes::ScreenHeight / 2.0 - Above,
		-Distance));
}

void AUghStage::SetWind(int32 Wind)
{
	const FWeather& Weather = Wind == 0 ? Calm : Storm;
	Sun->SetLightColor(Weather.SunColor);
	Sun->SetIntensity(Weather.SunLux);
	SkyLight->SetIntensity(Weather.SkyIntensity);
	Fog->SetFogDensity(Weather.FogDensity);
	Fog->SetFogInscatteringColor(Weather.FogColor);
}
