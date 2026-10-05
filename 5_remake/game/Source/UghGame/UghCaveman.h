// The cavemen: the copters' pilots, the passengers in their cabins and on the rock.
#pragma once

#include "CoreMinimal.h"
#include "UghMetaHumans.h"
#include "UghRig.h"
#include "UghCaveman.generated.h"

class AActor;
class USceneComponent;
class USkeletalMeshComponent;

/**
 * What a caveman does (the actions of Blender/caveman_actions.py and Python/metahuman_actions.py, where each says
 * where his origin is): standing, sitting, pedalling, hanging, walking, waving, treading water, swimming, falling.
 */
enum class EUghCaveAction : uint8 { Idle, Sit, Pedal, Hang, Walk, Wave, Tread, Swim, Fall };

/**
 * The people of the play by their look (0 the pilots, 1 .. 3 the passengers of the logic's cargo looks: a man, a
 * woman, an old man): the photoreal MetaHumans of metahumans.ps1 (UghMetaHumans) where all of them are there, else
 * the caveman of Blender/caveman.py (UghAssets::Caveman) dressed for the look (hair, beard, colours of hair, fur and
 * skin by its material slots). A person is a holder component, its origin the action's, PersonHeight tall
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

	/** A new person of `Look` on `Owner`, hidden (show it with its children), without collision (Load first). */
	USceneComponent* Add(AActor* Owner, int32 Look) const;
	/** Removes `Person` (Add's) with its parts. */
	static void Remove(USceneComponent* Person);
	/** `Person` does `Action` over and over. */
	void Play(USceneComponent* Person, EUghCaveAction Action) const;
	/** `Person` holds `Action` at `Fraction` (0 .. 1) of its loop (the pedalling follows the crank). */
	void Hold(USceneComponent* Person, EUghCaveAction Action, double Fraction) const;

private:
	/** The model of `Person` and its rig. */
	TPair<USkeletalMeshComponent*, const FUghRig*> ModelOf(USceneComponent* Person) const;

	UPROPERTY() FUghRig Caveman;                    // its actions by EUghCaveAction; none with the MetaHumans
	UPROPERTY() TArray<FUghMetaHuman> MetaHumans;   // by look; none without all of them
};
