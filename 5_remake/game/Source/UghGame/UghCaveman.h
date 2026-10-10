// The cavemen: the copters' pilots, the passengers in their cabins and on the rock.
#pragma once

#include "CoreMinimal.h"
#include "UghMetaHumans.h"
#include "UghRig.h"
#include "UghCaveman.generated.h"

class AActor;
class USceneComponent;
class UMaterialInterface;
class USkeletalMeshComponent;
class UStaticMesh;

/**
 * What a caveman does (the actions of Blender/caveman_actions.py and Python/metahuman_actions.py, where each says
 * where his origin is): standing (looking about), sitting, pedalling, hanging, walking, waving, treading water,
 * swimming, falling, flailing (flung through the air into the sea), ducking (a copter flying low by him), cheering
 * (walking off delivered, his arms up; FUghLively).
 */
enum class EUghCaveAction : uint8 { Idle, Sit, Pedal, Hang, Walk, Wave, Tread, Swim, Fall, Flail, Duck, Cheer };

/**
 * The people of the play by their look (0 the pilots, 1 .. 3 the passengers of the logic's cargo looks: a man, a
 * woman, an old man): the photoreal MetaHumans of metahumans.ps1 (UghMetaHumans) where all of them are there, else
 * the caveman of Blender/caveman.py (UghAssets::Caveman) dressed for the look (hair, beard, colours of hair, fur and
 * skin by its material slots). The old man leans on a staff (in his right hand, upright; not in the cabin or the water).
 * A person is a holder component, its origin the action's, PersonHeight tall
 * (UghFigurePlace) whatever the model; Load finds the models, without any there are no people (the game shows clay
 * instead).
 */
USTRUCT()
struct FUghCaveman
{
	GENERATED_BODY()

	/** The pilots' look. */
	static constexpr int32 PilotLook = 0;
	/** Whether the logic's cargo look `Look` is a person (else the stone with eyes: 4). */
	static bool IsPassenger(int32 Look) { return Look >= 1 && Look < UE_ARRAY_COUNT(UghMetaHumans::Names); }
	/** The names of the actions (as the models name them), by EUghCaveAction. */
	static TConstArrayView<const TCHAR*> ActionNames();

	/** Finds the MetaHumans, else the caveman; false (and a log line) when neither is there. */
	bool Load();
	bool IsLoaded() const { return Caveman.IsLoaded() || !MetaHumans.IsEmpty(); }

	/**
	 * A person of `Look` on `Owner`, hidden (show it with its children), without collision (Load first): a spare one of
	 * that look (Stock, Release), else made now (a MetaHuman takes 20-50 ms: a hitch in the play).
	 */
	USceneComponent* Add(AActor* Owner, int32 Look) const;
	/** Gives `Person` (Add's) back: hidden and still, a spare for the next of its look. */
	void Release(USceneComponent* Person) const;
	/**
	 * Makes spare people of every passenger's look on `Owner` until there are `PerLook` of each (while a level is built
	 * in the black, so that none has to be made in the play).
	 */
	void Stock(AActor* Owner, int32 PerLook) const;
	/** `Person` does `Action` over and over. */
	void Play(USceneComponent* Person, EUghCaveAction Action) const;
	/** `Person` holds `Action` at `Fraction` (0 .. 1) of its loop (the pedalling follows the crank). */
	void Hold(USceneComponent* Person, EUghCaveAction Action, double Fraction) const;

private:
	/** A new person of `Index` (a look within the people's) on `Owner`, hidden. */
	USceneComponent* Make(AActor* Owner, int32 Index) const;
	static int32 LookOf(int32 Look);
	/** The tag of a person of `Look`. */
	static FName LookTag(int32 Look);
	/** A spare person's parts do not tick (its pose, its hair). */
	static void SetTicking(USceneComponent* Person, bool bTicking);
	/** The model of `Person` and its rig. */
	TPair<USkeletalMeshComponent*, const FUghRig*> ModelOf(USceneComponent* Person) const;
	/** Gives the old man `Person` (made, its model there) his staff, hidden. */
	void AddStaff(AActor* Owner, USceneComponent* Person) const;
	/** The old man `Person` holds his staff while he stands, walks, waves, ducks or cheers (else it is hidden). */
	static void HoldStaff(USceneComponent* Person, EUghCaveAction Action);

	UPROPERTY() FUghRig Caveman;                    // its actions by EUghCaveAction; none with the MetaHumans
	UPROPERTY() TArray<FUghMetaHuman> MetaHumans;   // by look; none without all of them
	mutable TArray<TWeakObjectPtr<USceneComponent>> Spares;   // hidden, of their owners (which keep them)
	UPROPERTY() mutable TObjectPtr<UStaticMesh> Staff;              // made with the first old man
	UPROPERTY() mutable TObjectPtr<UMaterialInterface> StaffWood;
};
