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
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UghAssets.h"
#include "UghElectricDreams.h"
#include "UghMaterials.h"
#include "UghMood.h"
#include "UghShapes.h"
#include "UghWater.h"

namespace
{
	/** A fog low over the water: how fast it thins upwards; the mist of a storm, much faster. */
	constexpr float FogFalloff = 0.3f, MistFalloff = 2.5f;
	/**
	 * The volumetric fog (the shafts of sunlight, the campfires' glow) reaches the cliff far from the camera (its
	 * cells start there); it scatters the light a little back towards the camera (the sun is behind it).
	 */
	constexpr float VolumetricStart = 4000.f, VolumetricDistance = 12000.f, VolumetricScattering = -0.15f;
	/**
	 * The sky's dome: this far around the middle of the screen, units; the fog does not reach it (FogReach, the camera
	 * never further than a few hundred metres from the middle), so that the sky shows above the haze of the open sea.
	 */
	constexpr double SkyRadius = 1000000, FogReach = 500000;
	static_assert(UghWater::OpenSea + 50000 < FogReach && FogReach + 50000 < SkyRadius);

	/** The camera's horizontal field of view and how much it looks down (degrees); room around the screen. */
	constexpr float FieldOfView = 30.f, LookDown = 4.f;
	constexpr double ScreenMargin = 1.08;

	/** The plants of the Electric Dreams sample in a storm: how much harder and faster they sway. */
	constexpr float StormStrength = 3.f, StormSpeed = 2.5f;
	const FName WindStrengths[] = { TEXT("Wind Strength"), TEXT("Wind Strength Plants") };
	const FName WindSpeeds[] = { TEXT("Wind Speed"), TEXT("Wind Speed Plants") };
	const FName WindDirection = TEXT("Wind Direction");

	/**
	 * The film look: a gentle bloom around the bright, a warm white balance, colours a little richer and more contrasted
	 * than the tone mapper's, the corners a little darker (as a lens has them).
	 */
	void SetLook(FPostProcessSettings& Settings)
	{
		Settings.bOverride_AutoExposureMinBrightness = Settings.bOverride_AutoExposureMaxBrightness = true;
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
	Fog->SetSecondFogHeightFalloff(MistFalloff);
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogStartDistance(VolumetricStart);
	Fog->SetVolumetricFogDistance(VolumetricDistance);
	Fog->SetVolumetricFogScatteringDistribution(VolumetricScattering);
	Fog->SetFogCutoffDistance(FogReach);

	Look = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Look"));
	Look->SetupAttachment(RootComponent);
	Look->bUnbound = true;
	SetLook(Look->Settings);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetConstraintAspectRatio(false);
}

void AUghStage::BeginPlay()
{
	Super::BeginPlay();
	if (UghAssets::Texture(UghAssets::SkyDay))
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
	}
	SetMood(UghMood::Of(0, 0), 0);
}

FUghCameraPose AUghStage::Fit(const FBox2D& Pixels, double Aspect)
{
	const double HalfTan = FMath::Tan(FMath::DegreesToRadians(FieldOfView / 2));
	const double Width = Pixels.GetSize().X * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Height = Pixels.GetSize().Y * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Distance = FMath::Max(Width / 2 / HalfTan, Height / 2 * Aspect / HalfTan);
	// looking down at the middle from above it
	const double Above = Distance * FMath::Tan(FMath::DegreesToRadians(LookDown)) / UghShapes::UnitsPerPixel;
	const FVector2D Middle = Pixels.GetCenter();
	FUghCameraPose Pose;
	Pose.Location = UghShapes::ToWorld(Middle.X, Middle.Y - Above, -Distance);
	Pose.Rotation = FRotator(-LookDown, -90, 0);   // looking along -Y (UghShapes)
	Pose.FieldOfView = FieldOfView;
	return Pose;
}

double AUghStage::ViewportAspect()
{
	FVector2D Viewport(16, 9);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(Viewport);
	}
	return Viewport.X > 0 && Viewport.Y > 0 ? Viewport.X / Viewport.Y : 16.0 / 9.0;
}

void AUghStage::SetCamera(const FUghCameraPose& Pose)
{
	Camera->SetWorldLocationAndRotation(Pose.Location, Pose.Rotation);
	Camera->SetFieldOfView(Pose.FieldOfView);
	Look->Settings.bOverride_MotionBlurAmount = Pose.MotionBlur >= 0;
	Look->Settings.MotionBlurAmount = FMath::Max(Pose.MotionBlur, 0.f);
	Look->Settings.bOverride_AutoExposureBias = Pose.ExposureBias != 0;
	Look->Settings.AutoExposureBias = Pose.ExposureBias;
}

void AUghStage::SetMood(const FUghMood& Mood, int32 Wind)
{
	Sun->SetRelativeRotation(FRotator(Mood.SunPitch, Mood.SunYaw, 0));
	Sun->SetLightColor(Mood.SunColor);
	Sun->SetIntensity(Mood.SunLux);
	Sun->SetVolumetricScatteringIntensity(Mood.Shafts);
	SkyLight->SetIntensity(Mood.SkyLight);
	Fog->SetFogDensity(Mood.FogDensity);
	Fog->SetFogInscatteringColor(Mood.FogColor);
	Fog->SetSecondFogDensity(Mood.Mist);
	Look->Settings.AutoExposureMinBrightness = Look->Settings.AutoExposureMaxBrightness = Mood.Exposure;
	UTexture* Sky = SkyMaterial ? UghAssets::Texture(Mood.Sky) : nullptr;
	if (Sky)
	{
		SkyMaterial->SetTextureParameterValue(UghMaterials::SkyParameter, Sky);
		SkyMaterial->SetVectorParameterValue(UghMaterials::ColorParameter, Mood.SkyTint);
		SkyMaterial->SetScalarParameterValue(UghMaterials::SkySeenParameter, Mood.SkySeen);
	}
	if (SkyDome)
	{
		SkyDome->SetVisibility(Sky != nullptr);
	}
	Atmosphere->SetVisibility(Sky == nullptr);
	SetFoliageWind(Wind);
}

void AUghStage::SetWater(double Surface)
{
	Fog->SetSecondFogHeightOffset(UghShapes::ToWorld(0, Surface, 0).Z - Fog->GetComponentLocation().Z);
}

FVector AUghStage::SunDirection() const
{
	return Sun->GetForwardVector();
}

void AUghStage::SetFoliageWind(int32 Wind)
{
	UMaterialParameterCollection* Collection = UghElectricDreams::IsCopied()
		? UghElectricDreams::Collection(UghElectricDreams::FoliageWind) : nullptr;
	UMaterialParameterCollectionInstance* Instance =
		Collection ? GetWorld()->GetParameterCollectionInstance(Collection) : nullptr;
	if (!Instance)
	{
		return;
	}
	auto Scale = [&](const FName& Name, float Factor)
	{
		if (const FCollectionScalarParameter* Parameter = Collection->GetScalarParameterByName(Name))
		{
			Instance->SetScalarParameterValue(Name, Parameter->DefaultValue * (Wind == 0 ? 1.f : Factor));
		}
	};
	for (const FName& Name : WindStrengths)
	{
		Scale(Name, StormStrength);
	}
	for (const FName& Name : WindSpeeds)
	{
		Scale(Name, StormSpeed);
	}
	if (const FCollectionVectorParameter* Parameter = Collection->GetVectorParameterByName(WindDirection))
	{
		// the sample's own way in a calm, along the storm's wind in a storm
		const FLinearColor Way = Wind == 0 ? Parameter->DefaultValue : FLinearColor(Wind, 0, 0, Parameter->DefaultValue.A);
		Instance->SetVectorParameterValue(WindDirection, Way);
		UE_LOG(LogTemp, Display, TEXT("UGH wind in the plants: %s, its way %s (the sample's %s)"),
			Wind == 0 ? TEXT("calm") : TEXT("a storm"), *Way.ToString(), *Parameter->DefaultValue.ToString());
	}
}
