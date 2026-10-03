// What moves in a level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghFigures.generated.h"

class FUghSprites;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The figures of the level, plasticine shapes in the slab of the play: the copters (body, rotor, who rides in them),
 * the passengers, the enemies, the bonus items and the raindrops; a passenger's speech bubble is a card with the
 * original's sprite (it shows the pad it wants to go to). Drawn between the views of two steps of the logic (render
 * interpolation); a figure that jumps further than a step can move is not interpolated.
 */
UCLASS()
class AUghFigures : public AActor
{
	GENERATED_BODY()

public:
	AUghFigures();

	/** Shows the figures between `Previous` and `Current` (Alpha 0 .. 1); none outside the play. */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, const FUghSprites& Sprites);

protected:
	virtual void BeginPlay() override;

private:
	/** A speech bubble to show: its sprite, its top left corner (pixels). */
	struct FBubble
	{
		int32 Sprite;
		FVector2D At;
	};

	/** The copters; who rides in them or hangs below goes to `OutRiders`. */
	void ShowCopters(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
		TArray<FTransform>& OutRiders);
	void ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
		const FUghSprites& Sprites, const TArray<FTransform>& Riders);
	void ShowBubbles(const TArray<FBubble>& Bubbles, const FUghSprites& Sprites);
	void ShowRain(const ugh_logic_view& Current);
	void Clear();

	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> CopterBodies;   // by player
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rotors;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Passengers;   // also the ones in or below a copter
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Enemies;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BonusItems;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Raindrops;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BubbleCards;   // made as many as needed, hidden when unused
	UPROPERTY() TMap<int32, TObjectPtr<UTexture2D>> SpriteTextures;
};
