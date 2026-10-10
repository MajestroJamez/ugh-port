#include "UghCaveman.h"

#include "Algo/Count.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "UghAssets.h"
#include "UghFigureLook.h"
#include "UghFigurePlace.h"
#include "UghMeshes.h"
#include "UghShapes.h"

namespace
{
	/** The actions as the models name them, by EUghCaveAction. */
	const TCHAR* const Actions[] = { TEXT("idle"), TEXT("sit"), TEXT("pedal"), TEXT("hang"), TEXT("walk"),
		TEXT("wave"), TEXT("tread"), TEXT("swim"), TEXT("fall"), TEXT("flail"), TEXT("duck"), TEXT("cheer") };
	static_assert(UE_ARRAY_COUNT(Actions) == static_cast<int32>(EUghCaveAction::Cheer) + 1);
	/** The colour of a glTF material Interchange imported. */
	const FName ColorFactor(TEXT("BaseColorFactor"));

	/** How the caveman of Blender/caveman.py looks: his hair (short or long), a beard or not, colours. */
	struct FCaveLook
	{
		bool bLongHair = false;
		bool bBeard = false;
		FLinearColor Hair, Fur, Skin;
	};

	/**
	 * By look: the pilot, a young caveman (dark hair, darker fur), a woman (long blond hair, reddish fur), an old man
	 * (white hair and beard).
	 */
	const FCaveLook CaveLooks[] = {
		{ false, true, FLinearColor(0.13f, 0.08f, 0.05f), FLinearColor::White, FLinearColor::White },
		{ false, false, FLinearColor(0.04f, 0.025f, 0.02f), FLinearColor(0.55f, 0.45f, 0.35f),
			FLinearColor(1.f, 0.95f, 0.9f) },
		{ true, false, FLinearColor(0.85f, 0.62f, 0.28f), FLinearColor(1.f, 0.45f, 0.35f), FLinearColor(1.f, 1.f, 1.f) },
		{ false, true, FLinearColor(0.92f, 0.92f, 0.9f), FLinearColor(0.7f, 0.66f, 0.6f),
			FLinearColor(0.95f, 0.86f, 0.8f) } };
	static_assert(UE_ARRAY_COUNT(CaveLooks) == UE_ARRAY_COUNT(UghMetaHumans::Names));

	/** The tag of the old man's staff. */
	const FName StaffTag(TEXT("UghStaff"));
	/** The bones that may hold it: a MetaHuman's right hand, the caveman's (as Interchange names Blender's). */
	const FName StaffHands[] = { FName(TEXT("hand_r")), FName(TEXT("hand.R")), FName(TEXT("hand_R")) };
	/** Its wood: pale, so that it shows against the dark rock. */
	const FLinearColor StaffColor(0.42f, 0.3f, 0.17f);

	/**
	 * The old man's staff (cm, upright, its grip at the origin): a crooked stick some 7 cm thick (the original's is a
	 * pixel, 10 cm) from the ground - the hand of one standing PersonHeight tall hangs some 70 cm up - to over his
	 * shoulder, a knob on top.
	 */
	UStaticMesh* MakeStaff(UObject* Outer)
	{
		struct FRing
		{
			double Z, Radius, Aside;
		};
		const FRing Rings[] = { { -72, 2.8, 0 }, { -40, 3.1, 1.5 }, { -8, 3.3, -0.5 }, { 24, 3.4, 1 },
			{ 44, 3.6, 2.5 }, { 50, 5, 3.5 }, { 57, 4.8, 3.5 }, { 61, 1.5, 3.2 } };
		constexpr int32 Sides = 7;
		TArray<UghMeshes::FVertex> Vertices;
		TArray<int32> Triangles;
		for (const FRing& Ring : Rings)
		{
			for (int32 Side = 0; Side < Sides; ++Side)
			{
				const double Angle = 2 * UE_DOUBLE_PI * Side / Sides;
				const FVector Out(FMath::Cos(Angle), FMath::Sin(Angle), 0);
				Vertices.Add({ FVector(Ring.Aside, 0, Ring.Z) + Out * Ring.Radius, FVector3f(Out), FVector3f::UpVector,
					FVector2f::ZeroVector, FVector2f::ZeroVector });
			}
		}
		for (int32 Ring = 0; Ring + 1 < UE_ARRAY_COUNT(Rings); ++Ring)
		{
			for (int32 Side = 0; Side < Sides; ++Side)
			{
				const int32 A = Ring * Sides + Side, B = Ring * Sides + (Side + 1) % Sides;
				Triangles.Append({ A, B, B + Sides, A, B + Sides, A + Sides });
			}
		}
		return UghMeshes::Build(Outer, Vertices, Triangles);
	}

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
		if (!MetaHumans.AddDefaulted_GetRef().Load(Look, Actions))
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
	if (Index == UghMetaHumans::OldMan)
	{
		AddStaff(Owner, Person);
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

void FUghCaveman::AddStaff(AActor* Owner, USceneComponent* Person) const
{
	USkeletalMeshComponent* Model = ModelOf(Person).Key;
	const FName* Hand = nullptr;
	for (const FName& Bone : StaffHands)
	{
		Hand = !Hand && Model->DoesSocketExist(Bone) ? &Bone : Hand;
	}
	if (!Hand)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGH the old man has no right hand: no staff"));
		return;
	}
	if (!Staff)
	{
		Staff = MakeStaff(GetTransientPackage());
		StaffWood = UghShapes::Clay(GetTransientPackage(), StaffColor);
	}
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
	Part->SetStaticMesh(Staff);
	Part->SetMaterial(0, StaffWood);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetVisibleInRayTracing(false);   // as the body (FUghMetaHuman)
	Part->SetUsingAbsoluteRotation(true);   // upright whatever the hand does
	Part->SetUsingAbsoluteScale(true);      // its own size whatever the body's
	Part->ComponentTags.Add(StaffTag);
	Part->SetVisibility(false);
	Part->SetupAttachment(Model, *Hand);
	Part->RegisterComponent();
	Owner->AddInstanceComponent(Part);
}

void FUghCaveman::HoldStaff(USceneComponent* Person, EUghCaveAction Action)
{
	if (!Person->ComponentHasTag(LookTag(UghMetaHumans::OldMan)))
	{
		return;
	}
	using enum EUghCaveAction;
	const bool bHeld = Action == Idle || Action == Walk || Action == Wave || Action == Duck || Action == Cheer;
	TArray<USceneComponent*> Parts;
	Person->GetChildrenComponents(true, Parts);
	for (USceneComponent* Part : Parts)
	{
		if (Part->ComponentHasTag(StaffTag))
		{
			Part->SetHiddenInGame(!bHeld);   // (the figures show a person with all its parts: SetVisibility)
		}
	}
}

void FUghCaveman::Play(USceneComponent* Person, EUghCaveAction Action) const
{
	const auto [Model, Rig] = ModelOf(Person);
	Rig->Play(Model, static_cast<int32>(Action));
	HoldStaff(Person, Action);
}

void FUghCaveman::Hold(USceneComponent* Person, EUghCaveAction Action, double Fraction) const
{
	const auto [Model, Rig] = ModelOf(Person);
	Rig->Hold(Model, static_cast<int32>(Action), Fraction);
	HoldStaff(Person, Action);
}
