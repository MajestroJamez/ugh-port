// The models of the figures of the play.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghCaveman.h"
#include "UghFigureActions.h"
#include "UghFrameClock.h"
#include "UghRig.h"
#include "UghSnort.h"
#include "UghFigureModels.generated.h"

class AActor;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** What shows an entity: its model's component (made the first time) and how far its action has gone. */
USTRUCT()
struct FUghFigureSlot
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<USceneComponent> Person;   // a passenger of the look Look (FUghCaveman)
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Rigged;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() FUghSnort Snort;   // a blower's
	int32 Look = -1;
	FUghFrameClock Clock;   // the action that follows its sprite's frames
	double Spin = 0;        // a tumbling stone, a turning bonus item: radians
	bool bShown = false;    // in this frame
};

/**
 * The figures besides the copters: the passengers as people of their look (FUghCaveman: MetaHumans or the caveman)
 * or the stone with eyes (a scanned mossy rock, else a boulder), the flyer as a pterodactyl, the walker as Jan's
 * triceratops (else a cartoon one), the blower as Jan's T-rex asleep snorting dust out of its nostrils (else a
 * puffing beast), the tree as a scanned hornbeam with a face carved into it (else a cartoon tree), the bonus items as
 * fruits and a stone tablet. Each entity keeps its own component; Show places it as its sprite (UghFigurePlace) and
 * lets it do its action (FUghFigureActions). A model that is not imported shows nothing: the caller shows clay
 * instead.
 */
USTRUCT()
struct FUghFigureModels
{
	GENERATED_BODY()

	/** A falling stone tumbles, a bonus item turns: turns a second. */
	static constexpr double TumbleTurns = 1.2, ItemTurns = 0.25;
	/**
	 * The flyer's wings are its material slot WingSlot, the light shines through them (UghMaterials::Membrane with the
	 * maps WingColor and WingNormal of Blender/pterodactyl.py); as imported without them.
	 */
	inline static const FName WingSlot = TEXT("membrane");
	inline static const TCHAR* WingColor = TEXT("pterodactyl_hide_color");
	inline static const TCHAR* WingNormal = TEXT("pterodactyl_hide_normal");
	/** Spare people of each passenger's look made ahead: a level shows a few of a look at once. */
	static constexpr int32 SparePeople = 3;

	/** Finds the imported models (a missing one is logged). */
	void Load();
	bool Has(EUghModel Model) const;

	/** Begins a frame: nothing is shown yet. */
	void Begin();
	/**
	 * Shows `Entity` (on `Owner`) doing `Action`, its sprite of `Size` px with its top left corner at `At` (pixels),
	 * moving `Velocity` px a step across, `Seconds` after the last frame, moved by `Offset` (world: a flung passenger);
	 * false when its model is missing.
	 */
	bool Show(AActor* Owner, const ugh_logic_entity& Entity, const FUghFigureAction& Action, const FVector2D& At,
		const FIntPoint& Size, double Velocity, double Seconds, const FVector& Offset = FVector::ZeroVector);
	/** Ends a frame: hides what was not shown in it. */
	void End();
	/** Makes spare people of every passenger's look on `Owner`, SparePeople of each (FUghCaveman::Stock). */
	void Stock(AActor* Owner) const;
	/** The skeletal mesh of a rigged model (the flyer, the walker, the blower, the tree); none when not imported. */
	USkeletalMesh* SkeletalMeshOf(EUghModel Model) const;

private:
	const FUghRig* RigOf(EUghModel Model) const;
	UStaticMesh* MeshOf(const FUghFigureAction& Action) const;
	USceneComponent* PersonOf(AActor* Owner, FUghFigureSlot& Slot, int32 Look) const;
	void Animate(FUghFigureSlot& Slot, const FUghFigureAction& Action, double Seconds);
	static void Turn(FUghFigureSlot& Slot, const FUghFigureAction& Action, double Velocity, double Seconds);

	UPROPERTY() FUghCaveman Caveman;
	UPROPERTY() FUghRig Flyer;
	UPROPERTY() FUghRig Walker;
	UPROPERTY() FUghRig Blower;
	UPROPERTY() FUghRig Tree;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Wings;   // the flyer's (UghMaterials::Membrane)
	UPROPERTY() TObjectPtr<UStaticMesh> Stone;
	UPROPERTY() TMap<FString, TObjectPtr<UStaticMesh>> Items;   // by the kind of bonus item
	UPROPERTY() TMap<int32, FUghFigureSlot> Slots;              // by the entity's kind and index
};
