// What moves in a level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghBubbles.h"
#include "UghFigureModels.h"
#include "UghLively.h"
#include "UghStoneDrop.h"
#include "UghFigures.generated.h"

class FUghFlings;
class FUghSprites;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The figures of the level besides the copters (AUghCopters) in the slab of the play: the passengers, the enemies and
 * the bonus items as the models of the Blender scripts doing what their sprites say (FUghFigureModels,
 * FUghFigureActions), plasticine shapes where a model is not imported; a passenger's speech bubble is a card with
 * its sharp picture (UghBubbles: the pad it wants to go to, a question). A person on land is lively (FUghLively: looks
 * about, waves at a copter near, ducks under one flying low, walks off glad when delivered) where its sprite says. Drawn
 * between the views of two steps of the logic (UghBetween). The raindrops are AUghRain's.
 */
UCLASS()
class AUghFigures : public AActor
{
	GENERATED_BODY()

public:
	AUghFigures();

	/**
	 * Shows the figures between `Previous` and `Current` (Alpha 0 .. 1), `Seconds` after the last frame, doing what
	 * `Actions` say their sprites mean, and the clay passengers riding the clay copters (`ClayRiders`, boxes); none
	 * outside the play; the passengers knocked off their pads flung as `Flings` say (FUghFlings: flailing in the air).
	 */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& ClayRiders,
		const FUghFlings* Flings = nullptr);
	/**
	 * Learns what the speech bubbles among the `SpriteCount` sprites of `Logic`'s data show (UghBubbles::Look); keeps
	 * `Logic` for its collision mask (a stone let go never falls into the ground, UghSling).
	 */
	void LoadBubbles(const ugh_logic* Logic, int32 SpriteCount);
	/** Makes spare passengers ahead (while a level is built in the black): none is made in the play (FUghCaveman). */
	void Stock();
	/** What the passengers on land show besides their sprites (waving, ducking, glad: FUghLively). */
	const FUghLively& GetLively() const { return Lively; }

protected:
	virtual void BeginPlay() override;

private:
	/** A speech bubble to show: what it shows, where it is. */
	struct FBubble
	{
		FUghBubbleLook Look;
		FUghBubblePlace Place;
	};

	void ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& Riders,
		const FUghFlings* Flings);
	void ShowBubbles(const TArray<FBubble>& Bubbles);
	void Clear();

	UPROPERTY() FUghFigureModels Models;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Passengers;   // also the clay riders of the clay copters
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Enemies;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BonusItems;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BubbleCards;   // made as many as needed, hidden when unused
	UPROPERTY() TMap<int32, TObjectPtr<UTexture2D>> BubbleTextures;   // by the key of their look and side
	TArray<TOptional<FUghBubbleLook>> BubbleLooks;   // by sprite (LoadBubbles)
	FUghStoneDrops StoneDrops;   // the stones let go, seen falling from their slings
	const ugh_logic* Logic = nullptr;   // of LoadBubbles
	FUghLively Lively;           // the passengers on land waving, ducking, glad (their actions only)
};
