#include "UghCampfire.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	const FLinearColor LogColor(0.2f, 0.1f, 0.04f);
	/**
	 * Kenney's campfire in clay of these colours by its material slots (its own are pale, flat colours): stone,
	 * wood, woodDark.
	 */
	const FLinearColor StoneColor(0.1f, 0.095f, 0.09f), WoodColor(0.3f, 0.17f, 0.08f), CharredColor(0.1f, 0.05f, 0.03f);
	const FLinearColor FireColor(1.f, 0.42f, 0.1f);

	/**
	 * Parts of a fire's box: its stones and logs (Kenney's meshes fill its width; without them two clay logs this
	 * long and thick), the flame's cards (this wide) from the middle of the logs up to the top of the box, three
	 * crossed; the light at the flame's tip (lower, the stones would glare).
	 */
	constexpr double ClayLogLength = 0.9, ClayLogThick = 0.18, FlameWidth = 0.85;
	constexpr int32 FlameCards = 3;
	/** The light: its brightness (candela) and reach (units); how much and how fast it flickers. */
	constexpr float LightIntensity = 25.f, LightRadius = 900.f, FlickerSpeed = 7.f;
	constexpr float CalmFlicker = 0.25f, WindFlicker = 0.5f;
	/** In the wind the light is blown aside (pixels); the flames lean in their material. */
	constexpr double WindLightShift = 3;
	constexpr float FlameGlow = 1.2f;

	const TCHAR* PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");
}

AUghCampfire::AUghCampfire()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghCampfire::BeginPlay()
{
	Super::BeginPlay();
	HearthBounds.Init();
	for (UStaticMesh* Mesh : UghAssets::Meshes({ UghAssets::Campfire }))
	{
		HearthBounds += Mesh->GetBoundingBox();
		UInstancedStaticMeshComponent* Part = UghShapes::AddShapes(this, UghShapes::EShape::Cube, nullptr);
		Part->SetStaticMesh(Mesh);
		for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
		{
			const FString Name = Mesh->GetStaticMaterials()[Slot].MaterialSlotName.ToString();
			Part->SetMaterial(Slot, UghShapes::Clay(this, Name.Contains(TEXT("stone")) ? StoneColor
				: Name.Contains(TEXT("Dark")) ? CharredColor : WoodColor));
		}
		Hearths.Add(Part);
	}
	if (Hearths.IsEmpty())
	{
		Hearths.Add(UghShapes::AddShapes(this, UghShapes::EShape::Cylinder, UghShapes::Clay(this, LogColor)));
	}
	FlameMaterial = UghShapes::Material(this, UghMaterials::Fire);
	FlameMaterial->SetScalarParameterValue(UghMaterials::IntensityParameter, FlameGlow);
	Flames = UghShapes::AddShapes(this, UghShapes::EShape::Cube, FlameMaterial);
	Flames->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, PlaneMesh));
	Flames->SetCastShadow(false);
	Flames->SetVisibleInRayTracing(false);
}

void AUghCampfire::Place(const TArray<FUghDecoration>& Decorations, int32 InWind)
{
	Fires = Decorations.FilterByPredicate(
		[](const FUghDecoration& Decoration) { return Decoration.Kind == FUghDecoration::EKind::Campfire; });
	Wind = InWind;
	Flicker = Wind == 0 ? CalmFlicker : WindFlicker;
	FlameMaterial->SetScalarParameterValue(UghMaterials::WindParameter, Wind);
	Burning.Init(true, Fires.Num());
	while (Lights.Num() < Fires.Num())
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetLightColor(FireColor);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetAttenuationRadius(LightRadius);
		Light->RegisterComponent();
		AddInstanceComponent(Light);
		Lights.Add(Light);
	}
	Show();
}

void AUghCampfire::Show()
{
	TArray<FTransform> Bases, Cards;
	for (int32 Index = 0; Index < Lights.Num(); ++Index)
	{
		const bool bLit = Fires.IsValidIndex(Index) && Burning[Index];
		Lights[Index]->SetVisibility(bLit);
		if (!bLit)
		{
			continue;
		}
		const FUghDecoration& Fire = Fires[Index];
		const double Width = Fire.Width * UghShapes::UnitsPerPixel, Height = Fire.Height * UghShapes::UnitsPerPixel;
		const FVector Foot = UghShapes::ToWorld(Fire.X, Fire.Y, Fire.Depth);
		const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(Fire.Yaw));
		double BaseHeight;
		if (HearthBounds.IsValid)
		{
			// the stones and logs filling the box's width, standing on its foot
			const FVector Size = HearthBounds.GetSize();
			const double Scale = Width / FMath::Max(Size.X, Size.Y);
			const FVector Anchor(HearthBounds.GetCenter().X, HearthBounds.GetCenter().Y, HearthBounds.Min.Z);
			Bases.Add(FTransform(Turn, Foot - Turn.RotateVector(Anchor * Scale), FVector(Scale)));
			BaseHeight = Size.Z * Scale;
		}
		else
		{
			// two clay logs crossed: cylinders (along Z) laid down and turned both ways
			BaseHeight = Width * ClayLogThick;
			const FVector Scale(BaseHeight / UghShapes::ShapeSize, BaseHeight / UghShapes::ShapeSize,
				Width * ClayLogLength / UghShapes::ShapeSize);
			const FVector Centre = Foot + FVector(0, 0, BaseHeight / 2);
			Bases.Append({ FTransform(FRotator(90, Fire.Yaw + 30, 0), Centre, Scale),
				FTransform(FRotator(90, Fire.Yaw - 30, 0), Centre, Scale) });
		}
		// the flame: vertical cards (the plane turned up about X) crossed about the vertical, from the logs' middle
		const double Bottom = BaseHeight / 2, Flame = Height - Bottom;
		const FVector CardScale(Width * FlameWidth / UghShapes::ShapeSize, Flame / UghShapes::ShapeSize, 1);
		for (int32 Card = 0; Card < FlameCards; ++Card)
		{
			Cards.Add(FTransform(FRotator(0, Fire.Yaw + 180.0 * Card / FlameCards, 90),
				Foot + FVector(0, 0, Bottom + Flame / 2), CardScale));
		}
		Lights[Index]->SetWorldLocation(Foot + FVector(WindLightShift * UghShapes::UnitsPerPixel * Wind, 0,
			Bottom + Flame));
	}
	for (UInstancedStaticMeshComponent* Part : Hearths)
	{
		UghShapes::SetShapes(Part, Bases);
	}
	UghShapes::SetShapes(Flames, Cards);
	SetActorTickEnabled(!Cards.IsEmpty());
}

void AUghCampfire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	for (int32 Index = 0; Index < Fires.Num(); ++Index)
	{
		// each its own way
		const float Wobble = FMath::PerlinNoise1D(Time * FlickerSpeed + Index * 7.31f) * Flicker;
		Lights[Index]->SetIntensity(LightIntensity * (1 + Wobble));
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
