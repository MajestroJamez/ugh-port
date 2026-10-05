// The MetaHumans of the play.
#pragma once

#include "CoreMinimal.h"
#include "UghLeaves.h"
#include "UghRig.h"
#include "UghMetaHumans.generated.h"

class AActor;
class UBlueprintGeneratedClass;
class USCS_Node;
class USceneComponent;
class USkeletalMeshComponent;

/**
 * The photoreal people of the play, made with MetaHuman Creator by metahumans.ps1 (Python/metahumans.py: presets
 * as stone age people without clothes, assembled to <Root>/<name>; Python/metahuman_actions.py: their actions to
 * <Root>/<name>/Actions/AS_<action>). Not in git; without them the game shows the caveman of Blender/caveman.py.
 */
namespace UghMetaHumans
{
	inline const TCHAR* Root = TEXT("/Game/External/MetaHumans");
	/** By look: 0 the copters' pilot, 1 .. 3 the passengers of the logic's cargo looks (a man, a woman, an old man). */
	inline const TCHAR* const Names[] = { TEXT("Pilot"), TEXT("Man"), TEXT("Woman"), TEXT("Grandpa") };
	/** By look: whether the leaves cover the chest too (the woman's). */
	inline const bool Tops[] = { false, false, true, false };
	static_assert(UE_ARRAY_COUNT(Tops) == UE_ARRAY_COUNT(Names));
}

/**
 * A MetaHuman as metahumans.ps1 assembled it: the components of its blueprint (BP_<name>) without the actor - the
 * body playing an animation per action (a FUghRig), the face following its pose, the grooms on the face (hair and
 * beards as hair cards) - and the leaves it wears (FUghLeaves) on the body's bones. Load finds them; without any of
 * them there is none.
 */
USTRUCT()
struct FUghMetaHuman
{
	GENERATED_BODY()

	/**
	 * Finds the MetaHuman `Name` and its `ActionNames` (indexed as given) and makes its leaves (with a top when
	 * `bTop`); false (and a log line) when missing.
	 */
	bool Load(const TCHAR* Name, TConstArrayView<const TCHAR*> ActionNames, bool bTop);
	bool IsLoaded() const { return Body.IsLoaded(); }
	const FUghRig& GetBody() const { return Body; }
	/** Its height (cm): the top of its head. */
	double GetHeight() const { return OwnHeight; }

	const FUghLeaves& GetLeaves() const { return Leaves; }

	/**
	 * Its parts under `Holder` (of `Owner`), hidden: the body `Height` cm high (its origin the holder's), the face, the
	 * grooms, the leaves.
	 */
	void Add(AActor* Owner, USceneComponent* Holder, double Height) const;

private:
	struct FAdding
	{
		AActor* Owner;
		double Scale;
		USkeletalMeshComponent* Leader;
	};
	void AddNode(FAdding& Adding, const USCS_Node* Node, USceneComponent* Parent) const;

	UPROPERTY() TObjectPtr<UBlueprintGeneratedClass> Blueprint;
	UPROPERTY() FUghRig Body;
	UPROPERTY() FUghLeaves Leaves;
	double OwnHeight = 0;   // cm: the top of its head
};
