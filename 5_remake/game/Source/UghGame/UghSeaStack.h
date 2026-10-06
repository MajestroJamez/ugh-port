// The stone the level is carved into.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghSeaStack.generated.h"

class AUghScenery;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The sea stack the level is carved into (FUghStackField), flown to at the start of a level (FUghIntro), behind the
 * menu, and in the play its edges around the level (AUghStage::Play): a tower of grey karst limestone in the open sea,
 * in the cliff's material (AUghBackground; plain rock without it), wet at the water, its jungle on its top and ledges
 * (AUghScenery; hidden in the play, which does not see it: ShowJungle). Made the first time it shows (about a second),
 * then shown all along (hiding it at the end of the flight was a hitch, and the light changed), the same for every
 * level.
 */
UCLASS()
class AUghSeaStack : public AActor
{
	GENERATED_BODY()

public:
	AUghSeaStack();

	/** Shows the stone with its jungle or hides them. */
	void Show(bool bShow);
	bool IsShown() const { return bShown; }
	/**
	 * Its jungle shown or not (shown with the stone; out of the play's view, so the play need not draw its thousands of
	 * swaying plants): `bAtOnce` (in black), else a few of its kinds a frame (no hitch while the flight is seen).
	 */
	void ShowJungle(bool bShow, bool bAtOnce);
	/** The flight's seconds from which none of the jungle is in its view (test Ugh.Stack): it may hide then. */
	static constexpr double JungleOutOfView = 3.85;
	/** The water's surface, pixels from the top of the screen: the stone is wet there and under it. */
	void SetWater(double Surface);

private:
	void Make();

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Stone;   // none before it first shows
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> StoneMaterial;
	UPROPERTY() TObjectPtr<UTexture2D> StoneArt;
	UPROPERTY() TObjectPtr<AUghScenery> Jungle;
	bool bCliff = false;   // StoneMaterial is the cliff's
	bool bShown = false;
	bool bJungle = true;   // shown (with the stone)
};
