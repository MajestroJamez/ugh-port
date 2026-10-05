#include "UghEffects.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/RandomStream.h"
#include "UghMaterials.h"
#include "UghMeshes.h"
#include "UghShapes.h"
#include "UghWater.h"

namespace
{
	using namespace UghBursts;

	/** The quads are moved by their material far from where they are: the bounds' scale of a mesh of them (units). */
	constexpr float Reach = 3000;
	/** A flash's light: how far it reaches, how big its source is (units), how far in front of the burst. */
	constexpr float FlashRadius = 1500, FlashSource = 30;
	constexpr double FlashDepth = -60;
	/** A score rises from this far above its place (pixels). */
	constexpr double PopupAbove = 12;
	/** The parts of a burst at most (the key of its meshes). */
	constexpr int32 MaxParts = 16;

	const TCHAR* MaterialOf(EBlend Blend)
	{
		switch (Blend)
		{
		case EBlend::Bits: return UghMaterials::Bits;
		case EBlend::Glint: return UghMaterials::Glint;
		default: return UghMaterials::Burst;
		}
	}

	/** The parameters of a part (UghMaterials::Burst) for `Order`. */
	void SetPart(UMaterialInstanceDynamic* Material, const FPart& Part, const FUghEffectOrder& Order, float Floor,
		bool bRepeat)
	{
		FVector3f Direction = Part.Direction;
		if (Part.bFacing && !Order.bFacingLeft)
		{
			Direction.X = -Direction.X;   // the parts blow to the left of a figure facing left
		}
		const FLinearColor Color = Part.bTinted ? Part.Color * Order.Tint : Part.Color;
		const TPair<const TCHAR*, float> Scalars[] = {
			{ UghMaterials::LifeParameter, Part.Life }, { UghMaterials::StaggerParameter, Part.Stagger },
			{ UghMaterials::RepeatParameter, bRepeat ? 1.f : 0.f }, { UghMaterials::SpeedParameter, Part.Speed },
			{ UghMaterials::SpreadParameter, Part.Spread }, { UghMaterials::LiftParameter, Part.Lift },
			{ UghMaterials::GravityParameter, Part.Gravity }, { UghMaterials::DragParameter, Part.Drag },
			{ UghMaterials::SizeParameter, Part.Size }, { UghMaterials::GrowParameter, Part.Grow },
			{ UghMaterials::StretchParameter, Part.Stretch }, { UghMaterials::ModeParameter, float(Part.Mode) },
			{ UghMaterials::FlutterParameter, Part.Flutter }, { UghMaterials::SpinParameter, Part.Spin },
			{ UghMaterials::ScaleParameter, float(Order.Scale) }, { UghMaterials::FloorParameter, Floor },
			{ UghMaterials::ShapeParameter, float(Part.Shape) }, { UghMaterials::RoughnessParameter, Part.Roughness },
			{ Part.Blend == EBlend::Glint ? UghMaterials::IntensityParameter : UghMaterials::OpacityParameter,
				Part.Strength } };
		for (const TPair<const TCHAR*, float>& Scalar : Scalars)
		{
			Material->SetScalarParameterValue(Scalar.Key, Scalar.Value);
		}
		Material->SetVectorParameterValue(UghMaterials::DirectionParameter, FLinearColor(FVector(Direction)));
		Material->SetVectorParameterValue(UghMaterials::BoxParameter, FLinearColor(FVector(Part.Box)));
		Material->SetVectorParameterValue(UghMaterials::ColorParameter, Color);
	}
}

AUghEffects::AUghEffects()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AUghEffects::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds)
{
	for (const FUghEffectOrder& Order : Player.TakeOrders())
	{
		Start(Order);
	}
	WaterZ = float(UghShapes::ToWorld(0, UghWater::Surface(Previous, Current, Alpha), 0).Z);
	for (FUghBurstShown& Each : Shown)
	{
		if (!Each.bShown)
		{
			continue;
		}
		Each.Elapsed += Each.bHeld ? 0 : Seconds;
		const bool bLoop = Get(Each.Burst).bRepeat && Each.Owner != INDEX_NONE;
		const TOptional<FBox2D> Now = bLoop && Current.phase == UGH_LOGIC_PHASE_PLAY
			? Player.Find(Current, UGH_LOGIC_ENTITY_ENEMY, Each.Owner) : TOptional<FBox2D>();
		if ((bLoop && !Now) || (!Get(Each.Burst).bRepeat && Each.Elapsed > Lasts(Each.Burst)))
		{
			Hide(Each);
			continue;
		}
		if (Now)
		{
			// following its entity between the two steps
			const TOptional<FBox2D> Before = Player.Find(Previous, UGH_LOGIC_ENTITY_ENEMY, Each.Owner);
			const FVector2D At = Before ? FMath::Lerp(Before->GetCenter(), Now->GetCenter(), Alpha) : Now->GetCenter();
			for (UStaticMeshComponent* Part : Each.Parts)
			{
				Part->SetWorldLocation(UghShapes::ToWorld(At.X, At.Y, Get(Each.Burst).Depth));
			}
		}
		for (UMaterialInstanceDynamic* Material : Each.Materials)
		{
			Material->SetScalarParameterValue(UghMaterials::ElapsedParameter, float(Each.Elapsed));
			Material->SetScalarParameterValue(UghMaterials::WaterLevelParameter, WaterZ);
		}
	}
	Flare(Seconds);
	for (FPopup& Popup : Popups)
	{
		Popup.Age += Seconds;
	}
	Popups.RemoveAll([](const FPopup& Popup) { return Popup.Age > PopupSeconds; });
}

void AUghEffects::Hold(EUghBurst Burst, const FVector2D& Place, const ugh_logic_view& View, double Age)
{
	FUghEffectOrder Order = Player.OrderAt(Burst, Place, View);
	Order.bFacingLeft = true;   // as the parts' Direction is
	Order.Action = Get(Burst).bRepeat ? EUghEffectAction::Loop : EUghEffectAction::Play;
	if (FUghBurstShown* Held = Start(Order))
	{
		Held->Elapsed = Age;
		Held->bHeld = true;
	}
	for (FFlashing& Flashing : Flashes)
	{
		Flashing.Elapsed = Age;
		Flashing.bHeld = true;
	}
}

bool AUghEffects::IsHolding() const
{
	return Shown.ContainsByPredicate([](const FUghBurstShown& Each) { return Each.bShown && Each.bHeld; });
}

void AUghEffects::Clear()
{
	for (FUghBurstShown& Each : Shown)
	{
		Hide(Each);
	}
	Flashes.Reset();
	Flare(0);
	Popups.Reset();
}

FUghBurstShown* AUghEffects::Start(const FUghEffectOrder& Order)
{
	const FBurst& Burst = Get(Order.Burst);
	const bool bLoop = Order.Action == EUghEffectAction::Loop;
	auto IsOwners = [&Order](const FUghBurstShown& Each)
	{
		return Each.bShown && Each.Burst == Order.Burst && Each.Owner != INDEX_NONE && Each.Owner == Order.Owner;
	};
	if (Order.Action == EUghEffectAction::Stop || (bLoop && Shown.ContainsByPredicate(IsOwners)))
	{
		for (FUghBurstShown& Each : Shown)
		{
			if (Order.Action == EUghEffectAction::Stop && IsOwners(Each))
			{
				Hide(Each);
			}
		}
		return nullptr;
	}
	FUghBurstShown& Each = Take(Order.Burst);
	const FVector Where = UghShapes::ToWorld(Order.Place.X, Order.Place.Y, Burst.Depth);
	const float Floor = Order.Floor < TNumericLimits<double>::Max()
		? float(UghShapes::ToWorld(0, Order.Floor, 0).Z) : -1e6f;
	for (int32 Part = 0; Part < Each.Parts.Num(); ++Part)
	{
		SetPart(Each.Materials[Part], Burst.Parts[Part], Order, Floor, bLoop);
		Each.Materials[Part]->SetScalarParameterValue(UghMaterials::ElapsedParameter, 0.f);
		Each.Materials[Part]->SetScalarParameterValue(UghMaterials::WaterLevelParameter, WaterZ);
		Each.Parts[Part]->SetWorldLocation(Where);
		Each.Parts[Part]->SetVisibility(true);
	}
	Each.Elapsed = 0;
	Each.bShown = true;
	Each.bHeld = false;
	Each.Owner = bLoop ? Order.Owner : INDEX_NONE;
	if (Burst.Flash.Candelas > 0)
	{
		if (Flashes.Num() == MaxFlashes)
		{
			Flashes.RemoveAt(0);   // the oldest
		}
		UghBursts::FFlash Flash = Burst.Flash;
		Flash.Candelas *= float(Order.Scale);
		Flashes.Add({ Flash, UghShapes::ToWorld(Order.Place.X, Order.Place.Y, FlashDepth) });
	}
	if (Order.Points > 0)
	{
		Popups.Add({ UghShapes::ToWorld(Order.Place.X, Order.Place.Y - PopupAbove, 0), Order.Points, 0 });
	}
	return &Each;
}

FUghBurstShown& AUghEffects::Take(EUghBurst Burst)
{
	FUghBurstShown* Oldest = nullptr;
	int32 Count = 0;
	for (FUghBurstShown& Each : Shown)
	{
		if (Each.Burst != Burst)
		{
			continue;
		}
		if (!Each.bShown)
		{
			return Each;
		}
		++Count;
		Oldest = !Oldest || Each.Elapsed > Oldest->Elapsed ? &Each : Oldest;
	}
	if (Count >= MaxShown)
	{
		return *Oldest;
	}
	FUghBurstShown& New = Shown.AddDefaulted_GetRef();
	New.Burst = Burst;
	const TConstArrayView<FPart> Parts = Get(Burst).Parts;
	check(Parts.Num() <= MaxParts);
	for (int32 Part = 0; Part < Parts.Num(); ++Part)
	{
		UMaterialInstanceDynamic* Material = UghShapes::Material(this, MaterialOf(Parts[Part].Blend));
		UStaticMeshComponent* Mesh = UghMeshes::NewPart<UStaticMeshComponent>(this, Material);
		Mesh->SetStaticMesh(MeshOf(Burst, Part));
		Mesh->SetBoundsScale(Reach);
		Mesh->SetVisibleInRayTracing(false);
		Mesh->SetVisibility(false);
		New.Parts.Add(Mesh);
		New.Materials.Add(Material);
	}
	return New;
}

UStaticMesh* AUghEffects::MeshOf(EUghBurst Burst, int32 Part)
{
	const int32 Key = int32(Burst) * MaxParts + Part;
	if (const TObjectPtr<UStaticMesh>* Made = Meshes.Find(Key))
	{
		return *Made;
	}
	TArray<UghMeshes::FQuad> Quads;
	FRandomStream Random(Key + 1);
	for (int32 Each = 0; Each < Get(Burst).Parts[Part].Count; ++Each)
	{
		// (each a little aside in depth too: the bounds a box, not a plane the cliff's face could hide)
		const FVector Aside(0, Random.FRandRange(-0.5, 0.5), 0);
		Quads.Add({ Aside, FVector2D(1), FVector2D(Random.FRand(), Random.FRand()) });
	}
	UStaticMesh* Mesh = UghMeshes::Quads(this, Quads);
	Meshes.Add(Key, Mesh);
	return Mesh;
}

void AUghEffects::Hide(FUghBurstShown& Each)
{
	Each.bShown = false;
	Each.bHeld = false;
	for (UStaticMeshComponent* Part : Each.Parts)
	{
		Part->SetVisibility(false);
	}
}

void AUghEffects::Flare(double Seconds)
{
	while (Lights.Num() < MaxFlashes)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetAttenuationRadius(FlashRadius);
		Light->SetSourceRadius(FlashSource);
		Light->SetCastShadows(false);   // a moment's light: cheap
		Light->SetVisibility(false);
		Light->RegisterComponent();
		AddInstanceComponent(Light);
		Lights.Add(Light);
	}
	for (FFlashing& Flashing : Flashes)
	{
		Flashing.Elapsed += Flashing.bHeld ? 0 : Seconds;
	}
	Flashes.RemoveAll([](const FFlashing& Flashing) { return Flashing.Elapsed > Flashing.Flash.Seconds; });
	for (int32 Index = 0; Index < Lights.Num(); ++Index)
	{
		UPointLightComponent* Light = Lights[Index];
		Light->SetVisibility(Flashes.IsValidIndex(Index));
		if (Flashes.IsValidIndex(Index))
		{
			// at once, dying away
			const FFlashing& Flashing = Flashes[Index];
			const double Left = 1 - Flashing.Elapsed / Flashing.Flash.Seconds;
			Light->SetWorldLocation(Flashing.Where);
			Light->SetLightColor(Flashing.Flash.Color);
			Light->SetIntensity(Flashing.Flash.Candelas * float(Left * Left));
		}
	}
}
