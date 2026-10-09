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
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UghAssets.h"
#include "UghElectricDreams.h"
#include "UghFigureLook.h"
#include "UghMaterials.h"
#include "UghMood.h"
#include "UghShapes.h"
#include "UghWater.h"

namespace
{
	/**
	 * The figures' halo at all (1) or not (0): a post-process of 24 taps a pixel of the screen, after the upscaler. The
	 * quality presets set it (UghGraphics: none at low, the figures' lights alone set them apart there).
	 */
	TAutoConsoleVariable<int32> CVarHalo(TEXT("ugh.Halo"), 1,
		TEXT("Whether the figures have their soft dark halo (a post-process; 0: none, the pass not drawn)."));
	/**
	 * The scene's reflections of the engine (1) or none (0: no screen-space reflections on the rock and the plants - the
	 * sea's own still, r.Water.SingleLayer.Reflection 3). The quality presets set it (UghGraphics: none at low).
	 */
	TAutoConsoleVariable<int32> CVarSceneReflections(TEXT("ugh.SceneReflections"), 1,
		TEXT("Whether the scene has the engine's reflections (0: none - the sea's screen-space reflections alone)."));
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

	/**
	 * The figures' lights (UghFigureLook), the sun's colour half-way to white: a fill from the front right, a little from
	 * above (the sun comes from the front left), and a rim from behind them above, which lights their edges - their
	 * heads, shoulders and arms - against the rock, RimTimesFill times as bright as the fill.
	 */
	const FRotator FillFrom(-35, -115, 0), RimFrom(-40, 65, 0);
	constexpr float FigureLightTint = 0.5f, RimTimesFill = 2.f;

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

	// the figures' lights: on the figures alone, no shadows, nothing bounced by Lumen, not in the fog or the rain
	auto FigureLight = [this](const TCHAR* Name, const FRotator& From)
	{
		UDirectionalLightComponent* Light = CreateDefaultSubobject<UDirectionalLightComponent>(Name);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetRelativeRotation(From);
		Light->SetLightingChannels(false, true, false);
		Light->SetCastShadows(false);
		Light->SetIndirectLightingIntensity(0);
		Light->SetVolumetricScatteringIntensity(0);
		Light->SetAffectTranslucentLighting(false);
		Light->SetAtmosphereSunLight(false);
		return Light;
	};
	FigureFill = FigureLight(TEXT("FigureFill"), FillFrom);
	FigureRim = FigureLight(TEXT("FigureRim"), RimFrom);

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
	Halo = UghShapes::Material(this, UghMaterials::FigureHalo);
	Look->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1, Halo));
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

FUghCameraPose AUghStage::Play(double Aspect)
{
	// the screen and the stone around it (Fit adds its margin around the box)
	const FBox2D Screen = UghShapes::Screen();
	const FBox2D Seen(Screen.Min - FVector2D(StoneBeside, StoneAbove), Screen.Max + FVector2D(StoneBeside, StoneBelow));
	const FVector2D Middle = Seen.GetCenter(), Half = Seen.GetExtent() / ScreenMargin;
	return Fit(FBox2D(Middle - Half, Middle + Half), Aspect);
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
	ShowHalo();
	const bool bNoReflections = CVarSceneReflections.GetValueOnGameThread() == 0;
	Look->Settings.bOverride_ReflectionMethod = bNoReflections;
	Look->Settings.ReflectionMethod = bNoReflections ? EReflectionMethod::None : EReflectionMethod::Lumen;
}

void AUghStage::ShowHalo()
{
	const bool bShown = CVarHalo.GetValueOnGameThread() != 0;
	if (Halo && Look->Settings.WeightedBlendables.Array.Num() > 0 &&
		(Look->Settings.WeightedBlendables.Array[0].Weight > 0) != bShown)
	{
		Look->Settings.WeightedBlendables.Array[0].Weight = bShown ? 1 : 0;   // (none: the pass is not drawn)
	}
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
	const float Fill = Mood.FigureFill * FMath::Pow(2.f, Mood.Exposure);
	const FLinearColor FigureColor = FMath::Lerp(Mood.SunColor, FLinearColor::White, FigureLightTint);
	FigureFill->SetIntensity(Fill);
	FigureRim->SetIntensity(Fill * RimTimesFill);
	FigureFill->SetLightColor(FigureColor);
	FigureRim->SetLightColor(FigureColor);
	if (Halo)
	{
		Halo->SetScalarParameterValue(UghMaterials::DarkenParameter, Mood.Halo);
	}
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
