#include "UghCaveman.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "UghAssets.h"

namespace
{
	/** The actions as caveman.glb names them, by EUghCaveAction. */
	const TCHAR* const ActionNames[] = { TEXT("idle"), TEXT("sit"), TEXT("pedal"), TEXT("hang") };
	/** The colour of a glTF material Interchange imported. */
	const FName ColorFactor(TEXT("BaseColorFactor"));

	/** The passengers by the logic's cargo look: 1 a young caveman, 2 a woman with long hair, 3 an old man. */
	const FUghCaveLook Passengers[] = {
		{ false, false, FLinearColor(0.32f, 0.17f, 0.08f), FLinearColor(1.f, 0.85f, 0.7f), FLinearColor(1.f, 0.95f, 0.9f) },
		{ true, false, FLinearColor(0.55f, 0.2f, 0.06f), FLinearColor(1.f, 0.95f, 0.85f), FLinearColor(1.f, 1.f, 1.f) },
		{ false, true, FLinearColor(0.8f, 0.8f, 0.78f), FLinearColor(0.7f, 0.66f, 0.6f), FLinearColor(0.95f, 0.86f, 0.8f) } };

	int32 SlotOf(const USkeletalMesh* Mesh, const TCHAR* Name)
	{
		return Mesh->GetMaterials().IndexOfByPredicate(
			[&](const FSkeletalMaterial& Slot) { return Slot.MaterialSlotName == FName(Name); });
	}

	void Show(USkeletalMeshComponent* Caveman, const TCHAR* Name, bool bShow)
	{
		const USkeletalMesh* Mesh = Caveman->GetSkeletalMeshAsset();
		const int32 Slot = SlotOf(Mesh, Name);
		const TArray<FSkelMeshRenderSection>& Sections = Mesh->GetResourceForRendering()->LODRenderData[0].RenderSections;
		for (int32 Section = 0; Section < Sections.Num(); ++Section)
		{
			if (Sections[Section].MaterialIndex == Slot)
			{
				Caveman->ShowMaterialSection(Slot, Section, bShow, 0);
			}
		}
	}

	void Tint(USkeletalMeshComponent* Caveman, const TCHAR* Name, const FLinearColor& Color)
	{
		const int32 Slot = SlotOf(Caveman->GetSkeletalMeshAsset(), Name);
		if (UMaterialInstanceDynamic* Material = Caveman->CreateDynamicMaterialInstance(Slot))
		{
			Material->SetVectorParameterValue(ColorFactor, Color);
		}
	}
}

const FUghCaveLook FUghCaveman::Pilot{ false, true, FLinearColor(0.13f, 0.08f, 0.05f), FLinearColor::White,
	FLinearColor::White };

const FUghCaveLook* FUghCaveman::Passenger(int32 Look)
{
	return Look >= 1 && Look <= UE_ARRAY_COUNT(Passengers) ? &Passengers[Look - 1] : nullptr;
}

bool FUghCaveman::Load()
{
	Mesh = UghAssets::SkeletalMesh(UghAssets::Caveman);
	Actions.Reset();
	for (const TCHAR* Name : ActionNames)
	{
		Actions.Add(UghAssets::Animation(UghAssets::Caveman, Name));
	}
	if (Actions.Contains(nullptr))
	{
		Mesh = nullptr;
	}
	return IsLoaded();
}

USkeletalMeshComponent* FUghCaveman::Add(AActor* Owner) const
{
	USkeletalMeshComponent* Caveman = NewObject<USkeletalMeshComponent>(Owner);
	Caveman->SetSkeletalMesh(Mesh);
	Caveman->SetMobility(EComponentMobility::Movable);
	Caveman->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Caveman->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Caveman->SetVisibility(false);
	Caveman->SetupAttachment(Owner->GetRootComponent());
	Caveman->RegisterComponent();
	Owner->AddInstanceComponent(Caveman);
	return Caveman;
}

void FUghCaveman::Dress(USkeletalMeshComponent* Caveman, const FUghCaveLook& Look)
{
	Show(Caveman, TEXT("hair_short"), !Look.bLongHair);
	Show(Caveman, TEXT("hair_long"), Look.bLongHair);
	Show(Caveman, TEXT("beard"), Look.bBeard);
	Tint(Caveman, Look.bLongHair ? TEXT("hair_long") : TEXT("hair_short"), Look.Hair);
	Tint(Caveman, TEXT("beard"), Look.Hair);
	Tint(Caveman, TEXT("fur"), Look.Fur);
	Tint(Caveman, TEXT("skin"), Look.Skin);
}

void FUghCaveman::Play(USkeletalMeshComponent* Caveman, EUghCaveAction Action) const
{
	UAnimSequence* Sequence = Actions[static_cast<int32>(Action)];
	if (Caveman->GetSingleNodeInstance() == nullptr || Caveman->GetSingleNodeInstance()->GetAnimationAsset() != Sequence)
	{
		Caveman->PlayAnimation(Sequence, true);
	}
	Caveman->SetPlayRate(1.f);
}

void FUghCaveman::Hold(USkeletalMeshComponent* Caveman, EUghCaveAction Action, double Fraction) const
{
	UAnimSequence* Sequence = Actions[static_cast<int32>(Action)];
	Play(Caveman, Action);
	Caveman->SetPlayRate(0.f);
	Caveman->SetPosition(float(Fraction * Sequence->GetPlayLength()), false);
}
