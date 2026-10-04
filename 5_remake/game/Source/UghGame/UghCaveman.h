// The caveman: the copters' pilots, the passengers in their cabins.
#pragma once

#include "CoreMinimal.h"
#include "UghCaveman.generated.h"

class AActor;
class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;

/** How a caveman looks: his hair (short or long), a beard or not, and the colours of hair, fur and skin. */
struct FUghCaveLook
{
	bool bLongHair = false;
	bool bBeard = false;
	FLinearColor Hair = FLinearColor::White;
	FLinearColor Fur = FLinearColor::White;
	FLinearColor Skin = FLinearColor::White;

	bool operator==(const FUghCaveLook&) const = default;
};

/** What a caveman does (the actions of Blender/caveman_actions.py, where each says where his origin is). */
enum class EUghCaveAction : uint8 { Idle, Sit, Pedal, Hang };

/**
 * The caveman of Blender/caveman.py (UghAssets::Caveman): a skeletal mesh whose material slots are his parts (skin,
 * eye, pupil, hair_short, hair_long, beard, fur; the colours are the glTF base colour factors of their materials)
 * and his actions. Load finds them; without them there is no caveman (the game shows clay instead).
 */
USTRUCT()
struct FUghCaveman
{
	GENERATED_BODY()

	/** The pilots all look alike. */
	static const FUghCaveLook Pilot;
	/** The passenger of the logic's cargo look `Look` (ugh_logic_copter.cargo_look); none when he is no caveman. */
	static const FUghCaveLook* Passenger(int32 Look);

	/** Finds the imported caveman; false (and a log line) when something of him is missing. */
	bool Load();
	bool IsLoaded() const { return Mesh != nullptr; }

	/** A new caveman on `Owner`, hidden, without collision (Load first). */
	USkeletalMeshComponent* Add(AActor* Owner) const;
	/** Dresses `Caveman` as `Look`: the hair and beard it has, the colours. */
	static void Dress(USkeletalMeshComponent* Caveman, const FUghCaveLook& Look);
	/** `Caveman` does `Action` over and over. */
	void Play(USkeletalMeshComponent* Caveman, EUghCaveAction Action) const;
	/** `Caveman` holds `Action` at `Fraction` (0 .. 1) of its loop (the pedalling follows the crank). */
	void Hold(USkeletalMeshComponent* Caveman, EUghCaveAction Action, double Fraction) const;

private:
	UPROPERTY() TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Actions;   // by EUghCaveAction
};
