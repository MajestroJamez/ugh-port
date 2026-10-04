// The models of the figures of the play.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghCaveman.h"
#include "UghFigureActions.h"
#include "UghFrameClock.h"
#include "UghRig.h"
#include "UghFigureModels.generated.h"

class AActor;
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
	int32 Look = -1;
	FUghFrameClock Clock;   // the action that follows its sprite's frames
	double Spin = 0;        // a tumbling stone, a turning bonus item: radians
	bool bShown = false;    // in this frame
};

/**
 * The figures besides the copters: the passengers as people of their look (FUghCaveman: MetaHumans or the caveman)
 * or the stone with eyes, the flyer as a pterodactyl, the walker as a triceratops, the blower as a
 * puffing beast, the tree as a tree with a face, the bonus items as fruits and a stone tablet. Each entity keeps its
 * own component; Show places it as its sprite (UghFigurePlace) and lets it do its action (FUghFigureActions). A model
 * that is not imported shows nothing: the caller shows clay instead.
 */
USTRUCT()
struct FUghFigureModels
{
	GENERATED_BODY()

	/** A falling stone tumbles, a bonus item turns: turns a second. */
	static constexpr double TumbleTurns = 1.2, ItemTurns = 0.25;

	/** Finds the imported models (a missing one is logged). */
	void Load();
	bool Has(EUghModel Model) const;

	/** Begins a frame: nothing is shown yet. */
	void Begin();
	/**
	 * Shows `Entity` (on `Owner`) doing `Action`, its sprite of `Size` px with its top left corner at `At` (pixels),
	 * moving `Velocity` px a step across, `Seconds` after the last frame; false when its model is missing.
	 */
	bool Show(AActor* Owner, const ugh_logic_entity& Entity, const FUghFigureAction& Action, const FVector2D& At,
		const FIntPoint& Size, double Velocity, double Seconds);
	/** Ends a frame: hides what was not shown in it. */
	void End();

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
	UPROPERTY() TObjectPtr<UStaticMesh> Stone;
	UPROPERTY() TMap<FString, TObjectPtr<UStaticMesh>> Items;   // by the kind of bonus item
	UPROPERTY() TMap<int32, FUghFigureSlot> Slots;              // by the entity's kind and index
};
