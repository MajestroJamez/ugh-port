#include "UghStage.h"

#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	/** The sun from the front above and a little from the left (degrees). */
	const FRotator SunDirection(-45, -65, 0);
	/** A fog low over the water (the volumetric fog lets the campfire's light glow): how fast it thins upwards. */
	constexpr float FogFalloff = 0.3f;
	/** The sky's dome: this far around the middle of the screen, units. */
	constexpr double SkyRadius = 100000;

	/** The light and the air of a weather: the sun (colour, lux), the sky (light, tint of its picture), the fog. */
	struct FWeather
	{
		FLinearColor SunColor;
		float SunLux, SkyLight;
		FLinearColor SkyTint;
		float FogDensity;
		FLinearColor FogColor;
	};
	/** Calm: a warm evening sun, the evening sky, a thin fog. */
	const FWeather Calm{ FLinearColor(1.f, 0.85f, 0.68f), 8.f, 1.5f, FLinearColor(1.f, 1.f, 1.f), 0.01f,
		FLinearColor(0.45f, 0.5f, 0.6f) };
	/** The wind of a level brings a storm: a dim cool sun, a dark cloudy sky, a dense grey fog. */
	const FWeather Storm{ FLinearColor(0.7f, 0.8f, 1.f), 3.5f, 1.6f, FLinearColor(0.25f, 0.28f, 0.33f), 0.05f,
		FLinearColor(0.3f, 0.33f, 0.38f) };

	/** The camera's horizontal field of view and how much it looks down (degrees); room around the screen. */
	constexpr float FieldOfView = 30.f, LookDown = 4.f;
	constexpr double ScreenMargin = 1.08;

	/** The exposure, EV100: fixed, so that every level and weather is as bright as its light. */
	constexpr float Exposure = 1.5f;

	/**
	 * The film look: a gentle bloom around the bright, a warm white balance, colours a little richer and more contrasted
	 * than the tone mapper's, the corners a little darker (as a lens has them).
	 */
	void SetLook(FPostProcessSettings& Settings)
	{
		Settings.bOverride_AutoExposureMinBrightness = Settings.bOverride_AutoExposureMaxBrightness = true;
		Settings.AutoExposureMinBrightness = Settings.AutoExposureMaxBrightness = Exposure;
		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.35f;
		Settings.bOverride_WhiteTemp = true;
		Settings.WhiteTemp = 6100.f;
		Settings.bOverride_ColorSaturation = Settings.bOverride_ColorContrast = true;
		Settings.ColorSaturation = FVector4(1.08f, 1.08f, 1.08f, 1.f);
		Settings.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.f);
		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = 0.3f;
	}
}

AUghStage::AUghStage()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetRelativeRotation(SunDirection);
	Sun->SetAtmosphereSunLight(false);   // the atmosphere would tint it orange at this low angle: the cave keeps its colours

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);
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
	SetLook(Look->Settings);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetFieldOfView(FieldOfView);
	Camera->SetConstraintAspectRatio(false);
	Camera->SetRelativeRotation(FRotator(-LookDown, -90, 0));   // looking along -Y (UghShapes)
}

void AUghStage::BeginPlay()
{
	Super::BeginPlay();
	CalmSky = UghAssets::Texture(UghAssets::SkyCalm);
	StormSky = UghAssets::Texture(UghAssets::SkyStorm);
	if (CalmSky && StormSky)
	{
		// a sphere far around, seen from inside (the material is two-sided); the atmosphere behind it is not needed
		SkyDome = NewObject<UStaticMeshComponent>(this);
		SkyDome->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		SkyDome->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkyDome->SetCastShadow(false);
		SkyDome->SetVisibleInRayTracing(false);   // rays that miss the scene see the sky light, which captures it
		SkyDome->bAffectDynamicIndirectLighting = false;
		SkyDome->SetWorldLocation(UghShapes::ToWorld(UghShapes::ScreenWidth / 2.0, UghShapes::ScreenHeight / 2.0, 0));
		SkyDome->SetWorldScale3D(FVector(SkyRadius / (UghShapes::ShapeSize / 2)));
		SkyMaterial = UghShapes::Material(SkyDome, UghMaterials::Sky);
		SkyDome->SetMaterial(0, SkyMaterial);
		SkyDome->SetupAttachment(RootComponent);
		SkyDome->RegisterComponent();
		AddInstanceComponent(SkyDome);
		Atmosphere->SetVisibility(false);
	}
	SetWind(0);
}

void AUghStage::FitCamera(const FBox2D& Pixels)
{
	FVector2D Viewport(16, 9);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(Viewport);
	}
	const double Aspect = Viewport.Y > 0 ? Viewport.X / Viewport.Y : 16.0 / 9.0;
	const double HalfTan = FMath::Tan(FMath::DegreesToRadians(FieldOfView / 2));
	const double Width = Pixels.GetSize().X * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Height = Pixels.GetSize().Y * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Distance = FMath::Max(Width / 2 / HalfTan, Height / 2 * Aspect / HalfTan);
	// looking down at the middle from above it
	const double Above = Distance * FMath::Tan(FMath::DegreesToRadians(LookDown)) / UghShapes::UnitsPerPixel;
	const FVector2D Middle = Pixels.GetCenter();
	Camera->SetWorldLocation(UghShapes::ToWorld(Middle.X, Middle.Y - Above, -Distance));
}

void AUghStage::SetWind(int32 Wind)
{
	const FWeather& Weather = Wind == 0 ? Calm : Storm;
	Sun->SetLightColor(Weather.SunColor);
	Sun->SetIntensity(Weather.SunLux);
	SkyLight->SetIntensity(Weather.SkyLight);
	Fog->SetFogDensity(Weather.FogDensity);
	Fog->SetFogInscatteringColor(Weather.FogColor);
	if (SkyMaterial)
	{
		SkyMaterial->SetTextureParameterValue(UghMaterials::SkyParameter, Wind == 0 ? CalmSky : StormSky);
		SkyMaterial->SetVectorParameterValue(UghMaterials::ColorParameter, Weather.SkyTint);
	}
}
