#include "UghFigureModels.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "UghAssets.h"
#include "UghFigurePlace.h"

namespace
{
	int32 Key(const ugh_logic_entity& Entity) { return Entity.kind * UGH_LOGIC_MAX_ENTITIES + Entity.index; }

	UStaticMeshComponent* AddMesh(AActor* Owner)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner);
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetupAttachment(Owner->GetRootComponent());
		Mesh->RegisterComponent();
		Owner->AddInstanceComponent(Mesh);
		return Mesh;
	}
}

void FUghFigureModels::Load()
{
	Caveman.Load();
	Flyer.Load(UghAssets::Pterodactyl, FUghFigureActions::ActionsOf(EUghModel::Flyer));
	Walker.Load(UghAssets::Triceratops, FUghFigureActions::ActionsOf(EUghModel::Walker));
	Blower.Load(UghAssets::Blower, FUghFigureActions::ActionsOf(EUghModel::Blower));
	Tree.Load(UghAssets::FruitTree, FUghFigureActions::ActionsOf(EUghModel::Tree));
	Stone = UghAssets::Mesh(UghAssets::StonePassenger, TEXT("stone_passenger"));
	Items.Reset();
	for (UStaticMesh* Item : UghAssets::Meshes({ UghAssets::BonusItems }))
	{
		Items.Add(Item->GetName(), Item);
	}
}

bool FUghFigureModels::Has(EUghModel Model) const
{
	switch (Model)
	{
	case EUghModel::Caveman: return Caveman.IsLoaded();
	case EUghModel::Stone: return Stone != nullptr;
	case EUghModel::BonusItem: return !Items.IsEmpty();
	default: return RigOf(Model)->IsLoaded();
	}
}

const FUghRig* FUghFigureModels::RigOf(EUghModel Model) const
{
	switch (Model)
	{
	case EUghModel::Flyer: return &Flyer;
	case EUghModel::Walker: return &Walker;
	case EUghModel::Blower: return &Blower;
	case EUghModel::Tree: return &Tree;
	default: return nullptr;
	}
}

UStaticMesh* FUghFigureModels::MeshOf(const FUghFigureAction& Action) const
{
	if (Action.Model == EUghModel::Stone)
	{
		return Stone;
	}
	const TObjectPtr<UStaticMesh>* Item = Items.Find(Action.Item);
	return Item ? Item->Get() : nullptr;
}

void FUghFigureModels::Begin()
{
	for (TPair<int32, FUghFigureSlot>& Slot : Slots)
	{
		Slot.Value.bShown = false;
	}
}

bool FUghFigureModels::Show(AActor* Owner, const ugh_logic_entity& Entity, const FUghFigureAction& Action,
	const FVector2D& At, const FIntPoint& Size, double Velocity, double Seconds)
{
	const bool bPerson = Action.Model == EUghModel::Caveman;
	const bool bRigged = !bPerson && Action.Model != EUghModel::Stone && Action.Model != EUghModel::BonusItem;
	UStaticMesh* Mesh = bPerson || bRigged ? nullptr : MeshOf(Action);
	if (!Has(Action.Model) || (!bPerson && !bRigged && !Mesh))
	{
		return false;
	}
	FUghFigureSlot& Slot = Slots.FindOrAdd(Key(Entity));
	Slot.bShown = true;
	USceneComponent* Shown = nullptr;
	if (bPerson)
	{
		Shown = PersonOf(Owner, Slot, Entity.look);
	}
	else if (bRigged)
	{
		// a slot is an entity of one kind, but an enemy's model may change
		USkeletalMesh* Wanted = RigOf(Action.Model)->GetMesh();
		if (!Slot.Rigged)
		{
			Slot.Rigged = RigOf(Action.Model)->Add(Owner);
		}
		if (Slot.Rigged->GetSkeletalMeshAsset() != Wanted)
		{
			Slot.Rigged->SetSkeletalMesh(Wanted);
		}
		Shown = Slot.Rigged;
	}
	else
	{
		if (!Slot.Mesh)
		{
			Slot.Mesh = AddMesh(Owner);
		}
		Slot.Mesh->SetStaticMesh(Mesh);
		Shown = Slot.Mesh;
	}
	if (bPerson || bRigged)
	{
		Animate(Slot, Action, Seconds);
	}
	Turn(Slot, Action, Velocity, Seconds);
	Shown->SetWorldTransform(UghFigurePlace::Of(Action, At, Size, Slot.Clock.Phase(), Slot.Spin));
	USceneComponent* const Parts[] = { Slot.Person.Get(), Slot.Rigged.Get(), Slot.Mesh.Get() };
	for (USceneComponent* Part : Parts)
	{
		if (Part)
		{
			Part->SetVisibility(Part == Shown, true);
		}
	}
	return true;
}

/** The person of the passenger's look `Look` in `Slot`, made anew when the look changes. */
USceneComponent* FUghFigureModels::PersonOf(AActor* Owner, FUghFigureSlot& Slot, int32 Look) const
{
	if (Slot.Person && Slot.Look != Look)
	{
		FUghCaveman::Remove(Slot.Person);
		Slot.Person = nullptr;
	}
	if (!Slot.Person)
	{
		Slot.Person = Caveman.Add(Owner, FUghCaveman::IsPassenger(Look) ? Look : FUghCaveman::PilotLook);
		Slot.Look = Look;
	}
	return Slot.Person;
}

/** The action of a person or a rigged model: following the frames of its sprite, or playing by itself. */
void FUghFigureModels::Animate(FUghFigureSlot& Slot, const FUghFigureAction& Action, double Seconds)
{
	const int32 Index = FUghFigureActions::ActionsOf(Action.Model).Find(Action.Action);
	check(Index != INDEX_NONE);
	if (Action.bFollowsFrames)
	{
		Slot.Clock.Update(Action.Frame, Action.Frames, Seconds);
	}
	if (Action.Model == EUghModel::Caveman)
	{
		const EUghCaveAction CaveAction = static_cast<EUghCaveAction>(Index);
		if (Action.bFollowsFrames)
		{
			Caveman.Hold(Slot.Person, CaveAction, Slot.Clock.Phase());
		}
		else
		{
			Caveman.Play(Slot.Person, CaveAction);
		}
		return;
	}
	const FUghRig& Rig = *RigOf(Action.Model);
	if (Action.bFollowsFrames)
	{
		Rig.Hold(Slot.Rigged, Index, Slot.Clock.Phase());
	}
	else
	{
		Rig.Play(Slot.Rigged, Index);
	}
}

void FUghFigureModels::End()
{
	for (TPair<int32, FUghFigureSlot>& Slot : Slots)
	{
		if (!Slot.Value.bShown)
		{
			const FUghFigureSlot& Hidden = Slot.Value;
			USceneComponent* const Parts[] = { Hidden.Person.Get(), Hidden.Rigged.Get(), Hidden.Mesh.Get() };
			for (USceneComponent* Part : Parts)
			{
				if (Part)
				{
					Part->SetVisibility(false, true);
				}
			}
		}
	}
}

/** A falling stone tumbles the way it goes, a bonus item turns; the others do not. */
void FUghFigureModels::Turn(FUghFigureSlot& Slot, const FUghFigureAction& Action, double Velocity, double Seconds)
{
	if (Action.Model == EUghModel::BonusItem)
	{
		Slot.Spin += ItemTurns * UE_TWO_PI * Seconds;
	}
	else if (Action.Model == EUghModel::Stone)
	{
		const bool bFalling = FCString::Strcmp(Action.Action, TEXT("fall")) == 0;
		Slot.Spin = bFalling ? Slot.Spin + (Velocity < 0 ? -1 : 1) * TumbleTurns * UE_TWO_PI * Seconds : 0;
	}
}
