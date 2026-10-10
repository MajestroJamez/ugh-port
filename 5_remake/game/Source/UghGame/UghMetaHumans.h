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
	/**
	 * By look: the body's build, its scale across, in depth and up (the original's man is squat and broad, its woman
	 * slim; all as tall: UghFigurePlace::PersonHeight), so that the passengers' kinds are told apart at a glance from
	 * the game's camera (step 32c; who swims to safety and who drowns depends on it).
	 */
	inline const FVector Builds[] = { FVector(1), FVector(1.16, 1.12, 1), FVector(0.88, 0.9, 1), FVector(1) };
	static_assert(UE_ARRAY_COUNT(Builds) == UE_ARRAY_COUNT(Names));
	/**
	 * By look: the colour the leaves are tinted (the taro's green times it): the pilot's as they are, the man's dark,
	 * the woman's red, the old man's dry straw.
	 */
	inline const FLinearColor LeafTints[] = { FLinearColor::White, FLinearColor(0.38f, 0.32f, 0.22f),
		FLinearColor(2.6f, 0.2f, 0.35f), FLinearColor(1.8f, 1.1f, 2.2f) };
	static_assert(UE_ARRAY_COUNT(LeafTints) == UE_ARRAY_COUNT(Names));
	/** The look of the old man: he leans on a staff (FUghCaveman). */
	constexpr int32 OldMan = 3;
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
	 * Finds the MetaHuman of `Look` (UghMetaHumans::Names) and its `ActionNames` (indexed as given) and makes its leaves
	 * (with a top, tinted, by the look); false (and a log line) when missing.
	 */
	bool Load(int32 Look, TConstArrayView<const TCHAR*> ActionNames);
	bool IsLoaded() const { return Body.IsLoaded(); }
	const FUghRig& GetBody() const { return Body; }
	/** Its height (cm): the top of its head. */
	double GetHeight() const { return OwnHeight; }

	const FUghLeaves& GetLeaves() const { return Leaves; }

	/**
	 * Its parts under `Holder` (of `Owner`), hidden: the body `Height` cm high (its origin the holder's) in its look's
	 * build, the face, the grooms, the leaves.
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
	FVector Build = FVector::OneVector;   // UghMetaHumans::Builds
};
