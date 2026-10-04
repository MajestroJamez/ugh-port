// The MetaHumans of the play.
#pragma once

#include "CoreMinimal.h"
#include "UghRig.h"
#include "UghMetaHumans.generated.h"

class AActor;
class UBlueprintGeneratedClass;
class USCS_Node;
class USceneComponent;
class USkeletalMeshComponent;
class UMaterialInterface;

/**
 * The photoreal people of the play, made with MetaHuman Creator by metahumans.ps1 (Python/metahumans.py: presets
 * dressed as stone age people, assembled to <Root>/<name>; Python/metahuman_actions.py: their actions to
 * <Root>/<name>/Actions/AS_<action>). Not in git; without them the game shows the caveman of Blender/caveman.py.
 */
namespace UghMetaHumans
{
	inline const TCHAR* Root = TEXT("/Game/External/MetaHumans");
	/** By look: 0 the copters' pilot, 1 .. 3 the passengers of the logic's cargo looks (a man, a woman, an old man). */
	inline const TCHAR* const Names[] = { TEXT("Pilot"), TEXT("Man"), TEXT("Woman"), TEXT("Grandpa") };
}

/**
 * A MetaHuman as metahumans.ps1 assembled it: the components of its blueprint (BP_<name>) without the actor - the
 * body playing an animation per action (a FUghRig), the face and the outfit following its pose, the grooms on the
 * face (their helmets: hair cards are not assembled at the quality Low). Load finds them; without any of them there
 * is none.
 */
USTRUCT()
struct FUghMetaHuman
{
	GENERATED_BODY()

	/** Finds the MetaHuman `Name` and its `ActionNames` (indexed as given); false (and a log line) when missing. */
	bool Load(const TCHAR* Name, TConstArrayView<const TCHAR*> ActionNames);
	bool IsLoaded() const { return Body.IsLoaded(); }
	const FUghRig& GetBody() const { return Body; }
	/** Its height (cm): the top of its head. */
	double GetHeight() const { return OwnHeight; }

	/**
	 * Its parts under `Holder` (of `Owner`), hidden: the body `Height` cm high (its origin the holder's), the face, the
	 * outfit (of `Garment` where given), the grooms.
	 */
	void Add(AActor* Owner, USceneComponent* Holder, double Height, UMaterialInterface* Garment) const;

private:
	struct FAdding
	{
		AActor* Owner;
		double Scale;
		UMaterialInterface* Garment;
		USkeletalMeshComponent* Leader;
	};
	void AddNode(FAdding& Adding, const USCS_Node* Node, USceneComponent* Parent) const;

	UPROPERTY() TObjectPtr<UBlueprintGeneratedClass> Blueprint;
	UPROPERTY() FUghRig Body;
	double OwnHeight = 0;   // cm: the top of its head
};
