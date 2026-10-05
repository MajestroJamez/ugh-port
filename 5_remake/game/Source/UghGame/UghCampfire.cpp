#include "UghCampfire.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghFireParts.h"
#include "UghFlames.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	/**
	 * Parts of a fire's box (its width): the flame's cards (as tall as twice their width, as the flipbook's frames) from
	 * just above its foot, the light in the flame's lower half.
	 */
	constexpr double FlameWidth = 0.9, FlameFoot = 0.02, LightUp = 0.3;
	/** How bright the flame is; its sparks (how many a fire, how high and far aside they fly), its smoke. */
	constexpr float FlameGlow = 4.5f;
	constexpr int32 SparksAFire = 36, PuffsAFire = 18;
	constexpr float SparkRise = 170, SparkSpread = 45, SmokeRise = 320, SmokeSize = 45;
	/** The light: its brightness by day (candela) and reach (units), how soft its shadows; how much it flickers. */
	constexpr float LightIntensity = 9.f, LightRadius = 900.f, LightSource = 14.f;
	constexpr float CalmFlicker = 0.22f, WindFlicker = 0.4f;
	/** In the wind the light is blown aside (pixels); the flames lean in their material. */
	constexpr double WindLightShift = 3;
}

AUghCampfire::AUghCampfire()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCampfire::BeginPlay()
{
	Super::BeginPlay();
	Hearths.Make(this);
	Flames = UghFireParts::NewFlames(this, UghFlames::Campfire, FlameGlow, FlameMaterial);
	Sparks = UghFireParts::NewSparks(this, SparkRise, SparkSpread, SparkMaterial);
	Smoke = UghFireParts::NewSmoke(this, SmokeRise, SmokeSize, SmokeMaterial);
}

void AUghCampfire::Place(const TArray<FUghDecoration>& Decorations, int32 InWind, float FireLight)
{
	Fires = Decorations.FilterByPredicate(
		[](const FUghDecoration& Decoration) { return Decoration.Kind == FUghDecoration::EKind::Campfire; });
	Wind = InWind;
	Flicker = Wind == 0 ? CalmFlicker : WindFlicker;
	Candelas = LightIntensity * FireLight;
	for (UMaterialInstanceDynamic* Material : { FlameMaterial, SparkMaterial, SmokeMaterial })
	{
		Material->SetScalarParameterValue(UghMaterials::WindParameter, Wind);
	}
	Burning.Init(true, Fires.Num());
	LightPlaces.SetNum(Fires.Num());
	while (Lights.Num() < Fires.Num())
	{
		Lights.Add(UghFireParts::NewLight(this, LightRadius, LightSource));
	}
	Show();
}

void AUghCampfire::Show()
{
	TArray<FTransform> Cards;
	TArray<UghMeshes::FQuad> SparkQuads, Puffs;
	for (int32 Index = 0; Index < Lights.Num(); ++Index)
	{
		const bool bLit = Fires.IsValidIndex(Index) && Burning[Index];
		Lights[Index]->SetVisibility(bLit);
		if (!bLit)
		{
			continue;
		}
		const FUghDecoration& Fire = Fires[Index];
		const double Width = Fire.Width * UghShapes::UnitsPerPixel;
		const FVector Foot = UghShapes::ToWorld(Fire.X, Fire.Y, Fire.Depth);
		Hearths.Add(Foot, Width, Fire.Yaw, Fire.Variant);
		const double FlameHeight = 2 * FlameWidth * Width;
		const FVector FlameFootAt = Foot + FVector(0, 0, FlameFoot * Width);
		UghFireParts::AddFlame(Cards, FlameFootAt, FlameWidth * Width, FlameHeight);
		FRandomStream Random(Fire.Variant);
		UghFireParts::AddSparks(SparkQuads, FlameFootAt, Width, SparksAFire, Random);
		UghFireParts::AddSmoke(Puffs, FlameFootAt + FVector(0, 0, FlameHeight * 0.75), Width, PuffsAFire, Random);
		LightPlaces[Index] = FlameFootAt + FVector(WindLightShift * UghShapes::UnitsPerPixel * Wind, 0,
			LightUp * FlameHeight);
	}
	Hearths.Show();
	UghShapes::SetShapes(Flames, Cards);
	Sparks->SetStaticMesh(SparkQuads.IsEmpty() ? nullptr : UghMeshes::Quads(this, SparkQuads));
	Smoke->SetStaticMesh(Puffs.IsEmpty() ? nullptr : UghMeshes::Quads(this, Puffs));
	SetActorTickEnabled(!Cards.IsEmpty());
	Flare();
}

void AUghCampfire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	Flare();
}

void AUghCampfire::Flare()
{
	for (int32 Index = 0; Index < Fires.Num(); ++Index)
	{
		if (Burning[Index])
		{
			UghFireParts::Flare(Lights[Index], LightPlaces[Index],
				UghFireParts::FFlicker::At(Time, Fires[Index].Variant, Flicker), Candelas);
		}
	}
}

void AUghCampfire::SetWater(double Surface)
{
	bool bChanged = false;
	for (int32 Index = 0; Index < Fires.Num(); ++Index)
	{
		const bool bBurns = Fires[Index].Y < Surface;
		bChanged |= bBurns != Burning[Index];
		Burning[Index] = bBurns;
	}
	if (bChanged)
	{
		Show();
	}
}
