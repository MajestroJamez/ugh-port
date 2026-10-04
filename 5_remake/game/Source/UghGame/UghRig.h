// An animated model: a skeletal mesh and its actions.
#pragma once

#include "CoreMinimal.h"
#include "UghRig.generated.h"

class AActor;
class UAnimSequence;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * An animated model: a skeletal mesh and an animation per action. The rigged models of the Blender scripts (the
 * caveman, the pterodactyl, the triceratops, the blower, the tree: the skeletal mesh imported for an asset, its
 * animations named <mesh><action> by Interchange) or a MetaHuman's body and actions (FUghMetaHuman). Load finds them;
 * without the mesh or any of the actions there is no model (the game shows clay instead).
 */
USTRUCT()
struct FUghRig
{
	GENERATED_BODY()

	/** Finds the skeletal mesh of asset `Id` and its `ActionNames` (indexed as given); false (and a log line) when
	 * something is missing. */
	bool Load(const TCHAR* Id, TConstArrayView<const TCHAR*> ActionNames);
	/** The model `InMesh` with the animations `InActions` (indexed as given); false when one is missing. */
	bool Load(USkeletalMesh* InMesh, TConstArrayView<UAnimSequence*> InActions);
	bool IsLoaded() const { return Mesh != nullptr; }
	USkeletalMesh* GetMesh() const { return Mesh; }
	UAnimSequence* GetAction(int32 Action) const { return Actions[Action]; }

	/** A new component of the model on `Owner`, hidden, without collision (Load first). */
	USkeletalMeshComponent* Add(AActor* Owner) const;
	/** `Model` does action `Action` over and over. */
	void Play(USkeletalMeshComponent* Model, int32 Action) const;
	/** `Model` holds action `Action` at `Fraction` (0 .. 1) of its loop. */
	void Hold(USkeletalMeshComponent* Model, int32 Action, double Fraction) const;

private:
	UPROPERTY() TObjectPtr<USkeletalMesh> Mesh;
	UPROPERTY() TArray<TObjectPtr<UAnimSequence>> Actions;
};
