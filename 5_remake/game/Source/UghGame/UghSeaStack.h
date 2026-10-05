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
 * The sea stack the level is carved into (FUghStackField), seen while the camera flies to it at the start of a level
 * (FUghIntro): a tower of grey karst limestone in the open sea, in the cliff's material (AUghBackground; plain rock
 * without it), wet at the water, its jungle on its top and ledges (AUghScenery). Out of the game's view: hidden but
 * during the flight, so it costs the play nothing. Made the first time it shows (about a second), the same for every
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
};
