#include "UghCampfire.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	const FLinearColor LogColor(0.2f, 0.1f, 0.04f);
	const FLinearColor FireColor(1.f, 0.42f, 0.1f);

	/** In pixels: the logs, the flame above them; the light this high above the ledge. */
	constexpr double LogLength = 12, LogHeight = 2, FlameWidth = 6, FlameHeight = 10, LightHeight = 8;
	/** The light: its brightness (candela) and reach (units); how much and how fast it flickers. */
	constexpr float LightIntensity = 40.f, LightRadius = 900.f, Flicker = 0.25f, FlickerSpeed = 7.f;
	constexpr float FlameGlow = 4.f;
}

AUghCampfire::AUghCampfire()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCampfire::BeginPlay()
{
	Super::BeginPlay();
	Logs = UghShapes::AddShapes(this, UghShapes::EShape::Cylinder, UghShapes::Clay(this, LogColor));
	FlameMaterial = UghShapes::Material(this, UghMaterials::Fire);
	FlameMaterial->SetVectorParameterValue(UghMaterials::ColorParameter, FireColor);
	Flame = UghShapes::AddShapes(this, UghShapes::EShape::Cone, FlameMaterial);
	Flame->SetCastShadow(false);

	Light = NewObject<UPointLightComponent>(this);
	Light->SetupAttachment(RootComponent);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetLightColor(FireColor);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetAttenuationRadius(LightRadius);
	Light->RegisterComponent();
	AddInstanceComponent(Light);
	Place({});
}

void AUghCampfire::Place(const TOptional<FIntPoint>& Where)
{
	Hearth = Where;
	SetWater(UghShapes::ScreenHeight);
	if (!Hearth.IsSet())
	{
		return;
	}
	const double X = Hearth->X, Y = Hearth->Y;
	// two logs crossed: cylinders (100 units across and long, along Z) laid down and turned both ways
	const FVector Centre = UghShapes::ToWorld(X, Y - LogHeight / 2, 0);
	const double Across = LogHeight * UghShapes::UnitsPerPixel / UghShapes::ShapeSize,
		Along = LogLength * UghShapes::UnitsPerPixel / UghShapes::ShapeSize;
	const FVector Scale(Across, Across, Along);
	UghShapes::SetShapes(Logs,
		{ FTransform(FRotator(90, 30, 0), Centre, Scale), FTransform(FRotator(90, -30, 0), Centre, Scale) });
	UghShapes::SetShapes(Flame, { UghShapes::Box(X - FlameWidth / 2, Y - LogHeight - FlameHeight, FlameWidth, FlameHeight,
		0, FlameWidth * UghShapes::UnitsPerPixel) });
	Light->SetWorldLocation(UghShapes::ToWorld(X, Y - LightHeight, 0));
}

void AUghCampfire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	const float Wobble = FMath::PerlinNoise1D(Time * FlickerSpeed) * Flicker;
	Light->SetIntensity(LightIntensity * (1 + Wobble));
	FlameMaterial->SetScalarParameterValue(UghMaterials::IntensityParameter, FlameGlow * (1 + Wobble));
}

void AUghCampfire::SetWater(double Surface)
{
	const bool bBurns = Hearth.IsSet() && Surface > Hearth->Y;
	if (bBurns != bLit)
	{
		bLit = bBurns;
		SetActorHiddenInGame(!bLit);
		SetActorTickEnabled(bLit);
	}
}
