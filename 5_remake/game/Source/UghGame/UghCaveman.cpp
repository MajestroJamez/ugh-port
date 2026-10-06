#include "UghCaveman.h"

#include "Algo/Count.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "UghAssets.h"
#include "UghFigureLook.h"
#include "UghFigurePlace.h"

namespace
{
	/** The actions as the models name them, by EUghCaveAction. */
	const TCHAR* const Actions[] = { TEXT("idle"), TEXT("sit"), TEXT("pedal"), TEXT("hang"), TEXT("walk"),
		TEXT("wave"), TEXT("tread"), TEXT("swim"), TEXT("fall") };
	static_assert(UE_ARRAY_COUNT(Actions) == static_cast<int32>(EUghCaveAction::Fall) + 1);
	/** The colour of a glTF material Interchange imported. */
	const FName ColorFactor(TEXT("BaseColorFactor"));

	/** How the caveman of Blender/caveman.py looks: his hair (short or long), a beard or not, colours. */
	struct FCaveLook
	{
		bool bLongHair = false;
		bool bBeard = false;
		FLinearColor Hair, Fur, Skin;
	};

	/** By look: the pilot, a young caveman, a woman with long hair, an old man. */
	const FCaveLook CaveLooks[] = {
		{ false, true, FLinearColor(0.13f, 0.08f, 0.05f), FLinearColor::White, FLinearColor::White },
		{ false, false, FLinearColor(0.32f, 0.17f, 0.08f), FLinearColor(1.f, 0.85f, 0.7f), FLinearColor(1.f, 0.95f, 0.9f) },
		{ true, false, FLinearColor(0.55f, 0.2f, 0.06f), FLinearColor(1.f, 0.95f, 0.85f), FLinearColor(1.f, 1.f, 1.f) },
		{ false, true, FLinearColor(0.8f, 0.8f, 0.78f), FLinearColor(0.7f, 0.66f, 0.6f), FLinearColor(0.95f, 0.86f, 0.8f) } };
	static_assert(UE_ARRAY_COUNT(CaveLooks) == UE_ARRAY_COUNT(UghMetaHumans::Names));

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

	/** Dresses the caveman as `Look`: the hair and beard it has (material slots), the colours. */
	void Dress(USkeletalMeshComponent* Caveman, const FCaveLook& Look)
	{
		Show(Caveman, TEXT("hair_short"), !Look.bLongHair);
		Show(Caveman, TEXT("hair_long"), Look.bLongHair);
		Show(Caveman, TEXT("beard"), Look.bBeard);
		Tint(Caveman, Look.bLongHair ? TEXT("hair_long") : TEXT("hair_short"), Look.Hair);
		Tint(Caveman, TEXT("beard"), Look.Hair);
		Tint(Caveman, TEXT("fur"), Look.Fur);
		Tint(Caveman, TEXT("skin"), Look.Skin);
	}
}

TConstArrayView<const TCHAR*> FUghCaveman::ActionNames()
{
	return Actions;
}

bool FUghCaveman::Load()
{
	MetaHumans.Reset();
	for (int32 Look = 0; Look < UE_ARRAY_COUNT(UghMetaHumans::Names); ++Look)
	{
		if (!MetaHumans.AddDefaulted_GetRef().Load(UghMetaHumans::Names[Look], Actions, UghMetaHumans::Tops[Look]))
		{
			MetaHumans.Reset();
			break;
		}
	}
	if (!MetaHumans.IsEmpty())
	{
		return true;
	}
	UE_LOG(LogTemp, Display, TEXT("UGH no MetaHumans (metahumans.ps1): the caveman instead"));
	return Caveman.Load(UghAssets::Caveman, Actions);
}

USceneComponent* FUghCaveman::Add(AActor* Owner, int32 Look) const
{
	const int32 Index = LookOf(Look);
	for (int32 I = 0; I < Spares.Num(); ++I)
	{
		USceneComponent* Spare = Spares[I].Get();
		if (Spare && Spare->GetOwner() == Owner && Spare->ComponentHasTag(LookTag(Index)))
		{
			Spares.RemoveAtSwap(I);
			SetTicking(Spare, true);
			return Spare;
		}
	}
	return Make(Owner, Index);
}

void FUghCaveman::Release(USceneComponent* Person) const
{
	Person->SetVisibility(false, true);
	SetTicking(Person, false);
	Spares.Add(Person);
}

void FUghCaveman::Stock(AActor* Owner, int32 PerLook) const
{
	Spares.RemoveAll([](const TWeakObjectPtr<USceneComponent>& Spare) { return !Spare.IsValid(); });
	for (int32 Look = 1; IsPassenger(Look); ++Look)
	{
		const int32 Have = Algo::CountIf(Spares, [Owner, Look](const TWeakObjectPtr<USceneComponent>& Spare)
		{
			return Spare->GetOwner() == Owner && Spare->ComponentHasTag(LookTag(Look));
		});
		for (int32 Made = Have; Made < PerLook; ++Made)
		{
			USceneComponent* Person = Make(Owner, Look);
			SetTicking(Person, false);
			Spares.Add(Person);
		}
	}
}

int32 FUghCaveman::LookOf(int32 Look)
{
	return FMath::Clamp(Look, 0, UE_ARRAY_COUNT(CaveLooks) - 1);
}

FName FUghCaveman::LookTag(int32 Look)
{
	return FName(TEXT("UghLook"), Look + 1);
}

void FUghCaveman::SetTicking(USceneComponent* Person, bool bTicking)
{
	TArray<USceneComponent*> Parts;
	Person->GetChildrenComponents(true, Parts);
	for (USceneComponent* Part : Parts)
	{
		Part->SetComponentTickEnabled(bTicking);
	}
}

USceneComponent* FUghCaveman::Make(AActor* Owner, int32 Index) const
{
	const double Started = FPlatformTime::Seconds();
	USceneComponent* Person = NewObject<USceneComponent>(Owner);
	Person->SetMobility(EComponentMobility::Movable);
	Person->SetupAttachment(Owner->GetRootComponent());
	Person->RegisterComponent();
	Owner->AddInstanceComponent(Person);
	if (!MetaHumans.IsEmpty())
	{
		MetaHumans[Index].Add(Owner, Person, UghFigurePlace::PersonHeight);
	}
	else
	{
		USkeletalMeshComponent* Model = Caveman.Add(Owner);
		Model->AttachToComponent(Person, FAttachmentTransformRules::KeepRelativeTransform);
		Model->SetRelativeScale3D(FVector(UghFigurePlace::PersonHeight / UghFigurePlace::CavemanHeight));
		Dress(Model, CaveLooks[Index]);
	}
	UghFigureLook::Mark(Person);
	Person->ComponentTags.Add(LookTag(Index));
	Person->SetVisibility(false, true);
	const double Took = (FPlatformTime::Seconds() - Started) * 1000;
	UE_CLOG(Took > 5, LogTemp, Display, TEXT("UGH person of look %d made in %.0f ms"), Index, Took);
	return Person;
}

TPair<USkeletalMeshComponent*, const FUghRig*> FUghCaveman::ModelOf(USceneComponent* Person) const
{
	TArray<const FUghRig*> Rigs = { &Caveman };
	for (const FUghMetaHuman& Each : MetaHumans)
	{
		Rigs.Add(&Each.GetBody());
	}
	TArray<USceneComponent*> Parts;
	Person->GetChildrenComponents(false, Parts);
	for (USceneComponent* Part : Parts)
	{
		USkeletalMeshComponent* Model = Cast<USkeletalMeshComponent>(Part);
		for (const FUghRig* Rig : Rigs)
		{
			if (Model && Rig->IsLoaded() && Model->GetSkeletalMeshAsset() == Rig->GetMesh())
			{
				return { Model, Rig };
			}
		}
	}
	checkNoEntry();
	return {};
}

void FUghCaveman::Play(USceneComponent* Person, EUghCaveAction Action) const
{
	const auto [Model, Rig] = ModelOf(Person);
	Rig->Play(Model, static_cast<int32>(Action));
}

void FUghCaveman::Hold(USceneComponent* Person, EUghCaveAction Action, double Fraction) const
{
	const auto [Model, Rig] = ModelOf(Person);
	Rig->Hold(Model, static_cast<int32>(Action), Fraction);
}
