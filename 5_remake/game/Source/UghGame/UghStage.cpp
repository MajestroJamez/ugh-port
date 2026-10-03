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
	/** The sun: low and warm, from the front above and a little from the left (degrees, lux). */
	const FRotator SunDirection(-30, -70, 0);
	const FLinearColor SunColor(1.f, 0.88f, 0.72f);
	constexpr float SunLux = 4.f, SkyIntensity = 1.2f;
	/** A thin fog low over the water; the volumetric fog lets the campfire's light glow. */
	constexpr float FogDensity = 0.01f, FogFalloff = 0.3f;

	/** The camera's horizontal field of view and how much it looks down (degrees); room around the screen. */
	constexpr float FieldOfView = 30.f, LookDown = 4.f;
	constexpr double ScreenMargin = 1.08;

	/** Darker than the eye would choose: an evening, the campfire stands out. */
	constexpr float ExposureBias = -1.f;
}

AUghStage::AUghStage()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	UDirectionalLightComponent* Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(SunDirection);
	Sun->SetIntensity(SunLux);
	Sun->SetLightColor(SunColor);
	Sun->SetAtmosphereSunLight(false);   // the atmosphere would tint it orange at this low angle: the cave keeps its colours

	CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Sky"))->SetupAttachment(RootComponent);
	USkyLightComponent* SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(SkyIntensity);

	UExponentialHeightFogComponent* Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->SetFogDensity(FogDensity);
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
