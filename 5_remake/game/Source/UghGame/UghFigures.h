// What moves in a level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghFigureModels.h"
#include "UghFigures.generated.h"

class FUghSprites;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The figures of the level besides the copters (AUghCopters) in the slab of the play: the passengers, the enemies and
 * the bonus items as the models of the Blender scripts doing what their sprites say (FUghFigureModels,
 * FUghFigureActions), plasticine shapes where a model is not imported; the raindrops as strokes of clay; a
 * passenger's speech bubble is a card with the original's sprite (it shows the pad it wants to go to). Drawn between
 * the views of two steps of the logic (UghBetween).
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
	 * outside the play.
	 */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& ClayRiders);

protected:
	virtual void BeginPlay() override;

private:
	/** A speech bubble to show: its sprite, its top left corner (pixels). */
	struct FBubble
	{
		int32 Sprite;
		FVector2D At;
	};

	void ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds,
		const FUghSprites& Sprites, const FUghFigureActions& Actions, const TArray<FTransform>& Riders);
	void ShowBubbles(const TArray<FBubble>& Bubbles, const FUghSprites& Sprites);
	void ShowRain(const ugh_logic_view& Current);
	void Clear();

	UPROPERTY() FUghFigureModels Models;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Passengers;   // also the clay riders of the clay copters
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Enemies;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BonusItems;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Raindrops;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BubbleCards;   // made as many as needed, hidden when unused
	UPROPERTY() TMap<int32, TObjectPtr<UTexture2D>> SpriteTextures;
};
