#include "UghFireParts.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	/** The flicker: its noise's speeds (1/s) and parts; how far the light licks about (units, at Amount 1). */
	constexpr float Slow = 6.3f, Middle = 14.1f, Quick = 29.7f;
	constexpr float SlowPart = 0.6f, MiddlePart = 0.3f, QuickPart = 0.15f;
	const FVector Lick(9, 6, 14);
	/** The colour of a fire's light (kelvin) at its steady brightness, and how much redder or yellower it flickers. */
	constexpr float Kelvin = 1850, KelvinFlicker = 350;
	/** The sparks' and smoke puffs' meshes are seen this far around their quads (their material moves them). */
	constexpr float PartsReach = 25;

	const TCHAR* PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");

	/**
	 * How far the fires' lights lick about with their flames (0 .. 1): a light that moves draws its shadow maps anew every
	 * frame (the virtual shadow maps keep the pages of one that stays put). The quality presets set it (UghGraphics).
	 */
	TAutoConsoleVariable<float> CVarLick(TEXT("ugh.FireLights.Lick"), 1.f,
		TEXT("How far the lights of the campfires and torches lick about with their flames (0: they stay put)."));

	float Noise(double Time, float Speed, float Phase)
	{
		return FMath::PerlinNoise1D(float(FMath::Fmod(Time * Speed, 10000.0)) + Phase);
	}
}

UghFireParts::FFlicker UghFireParts::FFlicker::At(double Time, int32 Seed, float Amount)
{
	const float Phase = Seed * 7.31f;
	const float Wobble = SlowPart * Noise(Time, Slow, Phase) + MiddlePart * Noise(Time, Middle, Phase + 3.1f) +
		QuickPart * Noise(Time, Quick, Phase + 5.7f);
	FFlicker Flicker;
	Flicker.Brightness = FMath::Max(0.3f, 1 + 2 * Amount * Wobble);
	Flicker.Temperature = Kelvin + KelvinFlicker * (Flicker.Brightness - 1);
	Flicker.Offset = Lick * Amount * FVector(Noise(Time, Slow * 0.8f, Phase + 11), Noise(Time, Slow * 0.7f, Phase + 23),
		Noise(Time, Middle * 0.6f, Phase + 37));
	return Flicker;
}

UPointLightComponent* UghFireParts::NewLight(AActor* Owner, float Radius, float SourceRadius)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner);
	Light->SetupAttachment(Owner->GetRootComponent());
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetAttenuationRadius(Radius);
	Light->SetSourceRadius(SourceRadius);   // soft shadows: the flame is no point
	Light->SetSoftSourceRadius(SourceRadius);
	Light->SetUseTemperature(true);
	Light->SetCastShadows(true);
	Light->RegisterComponent();
	Owner->AddInstanceComponent(Light);
	return Light;
}

void UghFireParts::Flare(UPointLightComponent* Light, const FVector& Place, const FFlicker& Flicker, float Candelas)
{
	Light->SetWorldLocation(Place + Flicker.Offset * CVarLick.GetValueOnGameThread());
	Light->SetIntensity(Candelas * Flicker.Brightness);
	Light->SetTemperature(Flicker.Temperature);
}

UInstancedStaticMeshComponent* UghFireParts::NewFlames(AActor* Owner, const TCHAR* Flipbook, float Intensity,
	TObjectPtr<UMaterialInstanceDynamic>& OutMaterial)
{
	OutMaterial = UghShapes::Material(Owner, UghMaterials::Flame);
	OutMaterial->SetScalarParameterValue(UghMaterials::IntensityParameter, Intensity);
	const FString Object = FString::Printf(TEXT("%s.%s"), Flipbook, *FPackageName::GetShortName(Flipbook));
	if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Object))
	{
		OutMaterial->SetTextureParameterValue(UghMaterials::FlipbookParameter, Texture);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UGH no flipbook %s: run build.ps1 (UghMakeFlames bakes it)"), Flipbook);
	}
	UInstancedStaticMeshComponent* Flames = UghShapes::AddShapes(Owner, UghShapes::EShape::Cube, OutMaterial);
	Flames->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, PlaneMesh));
	Flames->SetCastShadow(false);
	Flames->SetVisibleInRayTracing(false);
	return Flames;
}

void UghFireParts::AddFlame(TArray<FTransform>& Cards, const FVector& Foot, double Width, double Height)
{
	// the plane turned up about X (u across, v down), facing the camera; the other one turned across it
	const FVector Scale(Width / UghShapes::ShapeSize, Height / UghShapes::ShapeSize, 1);
	const FVector Centre = Foot + FVector(0, 0, Height / 2);
	Cards.Add(FTransform(FRotator(0, 0, 90), Centre, Scale));
	Cards.Add(FTransform(FRotator(0, 90, 90), Centre, Scale));
}

UStaticMeshComponent* UghFireParts::NewSparks(AActor* Owner, float Rise, float Spread,
	TObjectPtr<UMaterialInstanceDynamic>& OutMaterial)
{
	OutMaterial = UghShapes::Material(Owner, UghMaterials::Sparks);
	OutMaterial->SetScalarParameterValue(UghMaterials::RiseParameter, Rise);
	OutMaterial->SetScalarParameterValue(UghMaterials::SpreadParameter, Spread);
	UStaticMeshComponent* Sparks = UghMeshes::NewPart<UStaticMeshComponent>(Owner, OutMaterial);
	Sparks->SetBoundsScale(PartsReach);
	Sparks->SetVisibleInRayTracing(false);
	return Sparks;
}

UStaticMeshComponent* UghFireParts::NewSmoke(AActor* Owner, float Rise, float Size,
	TObjectPtr<UMaterialInstanceDynamic>& OutMaterial)
{
	OutMaterial = UghShapes::Material(Owner, UghMaterials::Smoke);
	OutMaterial->SetScalarParameterValue(UghMaterials::RiseParameter, Rise);
	OutMaterial->SetScalarParameterValue(UghMaterials::SizeParameter, Size);
	UStaticMeshComponent* Smoke = UghMeshes::NewPart<UStaticMeshComponent>(Owner, OutMaterial);
	Smoke->SetBoundsScale(PartsReach);
	Smoke->SetVisibleInRayTracing(false);
	return Smoke;
}

void UghFireParts::AddSparks(TArray<UghMeshes::FQuad>& Quads, const FVector& Foot, double Width, int32 Count,
	FRandomStream& Random)
{
	for (int32 Spark = 0; Spark < Count; ++Spark)
	{
		const FVector From = Foot + FVector(Random.FRandRange(-0.3, 0.3) * Width, Random.FRandRange(-0.2, 0.2) * Width,
			Random.FRandRange(0.1, 0.3) * Width);
		Quads.Add({ From, FVector2D(1), FVector2D(Random.FRand(), Random.FRand()) });
	}
}

void UghFireParts::AddSmoke(TArray<UghMeshes::FQuad>& Quads, const FVector& Tip, double Width, int32 Count,
	FRandomStream& Random)
{
	for (int32 Puff = 0; Puff < Count; ++Puff)
	{
		const FVector From = Tip + FVector(Random.FRandRange(-0.15, 0.15) * Width, Random.FRandRange(-0.1, 0.1) * Width, 0);
		Quads.Add({ From, FVector2D(1), FVector2D(Random.FRand(), Random.FRand()) });
	}
}
