#include "UghTorches.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UghAssets.h"
#include "UghFireParts.h"
#include "UghFlames.h"
#include "UghMaterials.h"
#include "UghShapes.h"

namespace
{
	using EKind = FUghDecoration::EKind;

	/**
	 * The torch (units, as Blender/torch.py makes it): its shaft this long from its foot to the top of its head, the
	 * head this long and thick; it leans out of the wall towards the camera this much and aside up to LeanAside
	 * (degrees, each its own way).
	 */
	constexpr double ShaftLength = 75, HeadLength = 20, HeadThick = 10, ShaftThick = 4.4;
	constexpr double LeanOut = 22, LeanAside = 12;
	/** Its flame (units: as tall as twice its width, as the flipbook's frames) from this far down its head. */
	constexpr double FlameWidth = 30, FlameDown = 12;
	/** How bright the flame and the glowing head are; its sparks and puffs of smoke. */
	constexpr float FlameGlow = 6.f, HeadGlow = 1.2f, HeadUnderside = 0.3f;
	constexpr int32 SparksATorch = 10, PuffsATorch = 8;
	constexpr float SparkRise = 90, SparkSpread = 20, SmokeRise = 180, SmokeSize = 25;
	/** The light: brightness by day (candela), reach and softness (units), above the head and out of the wall. */
	constexpr float LightIntensity = 0.8f, LightRadius = 500.f, LightSource = 6.f;
	const FVector LightAbove(0, 20, 14);   // out of the wall (+Y) and up
	constexpr float CalmFlicker = 0.25f, WindFlicker = 0.45f;
	const FLinearColor ClayWood(0.25f, 0.15f, 0.08f);

	/** Which way `Torch`'s shaft goes from its foot: out of the wall (+Y, towards the camera), up and a little aside. */
	FVector Way(const FUghDecoration& Torch)
	{
		const double Aside = FMath::DegreesToRadians(LeanAside * ((Torch.Variant % 201) / 100.0 - 1));
		const double Out = FMath::DegreesToRadians(LeanOut);
		return FVector(FMath::Sin(Aside), FMath::Sin(Out), FMath::Cos(Out) * FMath::Cos(Aside)).GetSafeNormal();
	}

	FVector Foot(const FUghDecoration& Torch)
	{
		return UghShapes::ToWorld(Torch.X, Torch.Y, Torch.Back());
	}
}

AUghTorches::AUghTorches()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector AUghTorches::Head(const FUghDecoration& Torch)
{
	return Foot(Torch) + Way(Torch) * ShaftLength;
}

void AUghTorches::BeginPlay()
{
	Super::BeginPlay();
	UMaterialInstanceDynamic* Glowing = UghShapes::Material(this, UghMaterials::Embers);
	Glowing->SetScalarParameterValue(UghMaterials::GlowParameter, HeadGlow);
	Glowing->SetScalarParameterValue(UghMaterials::UndersideParameter, HeadUnderside);
	const TArray<UStaticMesh*> Models = UghAssets::Meshes({ UghAssets::Torch });
	if (UStaticMesh* Model = Models.IsEmpty() ? nullptr : Models[0])
	{
		Shafts = UghShapes::AddShapes(this, UghShapes::EShape::Cube, nullptr);
		Shafts->SetStaticMesh(Model);
		ModelScale = ShaftLength / FMath::Max(Model->GetBoundingBox().GetSize().Z, 1.0);   // whatever units it came in
		for (int32 Slot = 0; Slot < Model->GetStaticMaterials().Num(); ++Slot)
		{
			if (Model->GetStaticMaterials()[Slot].MaterialSlotName.ToString().Contains(TEXT("char")))
			{
				Shafts->SetMaterial(Slot, Glowing);
			}
		}
	}
	else
	{
		Shafts = UghShapes::AddShapes(this, UghShapes::EShape::Cylinder, UghShapes::Clay(this, ClayWood));
		ClayHeads = UghShapes::AddShapes(this, UghShapes::EShape::Sphere, Glowing);
	}
	Flames = UghFireParts::NewFlames(this, UghFlames::Torch, FlameGlow, FlameMaterial);
	Sparks = UghFireParts::NewSparks(this, SparkRise, SparkSpread, SparkMaterial);
	Smoke = UghFireParts::NewSmoke(this, SmokeRise, SmokeSize, SmokeMaterial);
}

void AUghTorches::Place(const TArray<FUghDecoration>& Decorations, int32 Wind, float FireLight)
{
	Torches = Decorations.FilterByPredicate(
		[](const FUghDecoration& Decoration) { return Decoration.Kind == EKind::Torch; });
	Flicker = Wind == 0 ? CalmFlicker : WindFlicker;
	Candelas = LightIntensity * FireLight;
	for (UMaterialInstanceDynamic* Material : { FlameMaterial, SparkMaterial, SmokeMaterial })
	{
		Material->SetScalarParameterValue(UghMaterials::WindParameter, Wind);
	}
	Burning.Init(true, Torches.Num());
	while (Lights.Num() < Torches.Num())
	{
		Lights.Add(UghFireParts::NewLight(this, LightRadius, LightSource));
	}
	Show();
}

void AUghTorches::Show()
{
	TArray<FTransform> Bodies, Heads, Cards;
	TArray<UghMeshes::FQuad> SparkQuads, Puffs;
	for (int32 Index = 0; Index < Lights.Num(); ++Index)
	{
		const bool bLit = Torches.IsValidIndex(Index) && Burning[Index];
		Lights[Index]->SetVisibility(bLit);
		if (!Torches.IsValidIndex(Index))
		{
			continue;
		}
		const FUghDecoration& Torch = Torches[Index];
		const FVector Along = Way(Torch), From = Foot(Torch), Top = Head(Torch);
		const FQuat Turn = FRotationMatrix::MakeFromZ(Along).ToQuat() *
			FQuat(FVector::UpVector, FMath::DegreesToRadians(Torch.Yaw));
		if (ClayHeads)
		{
			const double Thick = ShaftThick / UghShapes::ShapeSize;
			Bodies.Add(FTransform(Turn, From + Along * ShaftLength / 2,
				FVector(Thick, Thick, ShaftLength / UghShapes::ShapeSize)));
			Heads.Add(FTransform(Turn, Top - Along * HeadLength / 2,
				FVector(HeadThick, HeadThick, HeadLength) / UghShapes::ShapeSize));
		}
		else
		{
			Bodies.Add(FTransform(Turn, From, FVector(ModelScale)));
		}
		if (bLit)   // a torch under the water still stands there, out
		{
			const FVector FlameFoot = Top - Along * FlameDown;
			UghFireParts::AddFlame(Cards, FlameFoot, FlameWidth, 2 * FlameWidth);
			FRandomStream Random(Torch.Variant);
			UghFireParts::AddSparks(SparkQuads, FlameFoot, FlameWidth, SparksATorch, Random);
			UghFireParts::AddSmoke(Puffs, FlameFoot + FVector(0, 0, 1.6 * FlameWidth), FlameWidth, PuffsATorch, Random);
		}
	}
	UghShapes::SetShapes(Shafts, Bodies);
	if (ClayHeads)
	{
		UghShapes::SetShapes(ClayHeads, Heads);
	}
	UghShapes::SetShapes(Flames, Cards);
	Sparks->SetStaticMesh(SparkQuads.IsEmpty() ? nullptr : UghMeshes::Quads(this, SparkQuads));
	Smoke->SetStaticMesh(Puffs.IsEmpty() ? nullptr : UghMeshes::Quads(this, Puffs));
	SetActorTickEnabled(!Cards.IsEmpty());
	Flare();
}

void AUghTorches::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	Flare();
}

void AUghTorches::Flare()
{
	for (int32 Index = 0; Index < Torches.Num(); ++Index)
	{
		if (Burning[Index])
		{
			UghFireParts::Flare(Lights[Index], Head(Torches[Index]) + LightAbove,
				UghFireParts::FFlicker::At(Time, Torches[Index].Variant, Flicker), Candelas);
		}
	}
}

void AUghTorches::SetWater(double Surface)
{
	bool bChanged = false;
	for (int32 Index = 0; Index < Torches.Num(); ++Index)
	{
		const bool bBurns = Torches[Index].Top() < Surface;
		bChanged |= bBurns != Burning[Index];
		Burning[Index] = bBurns;
	}
	if (bChanged)
	{
		Show();
	}
}
