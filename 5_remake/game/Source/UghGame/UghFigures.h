// What moves in a level.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghFigures.generated.h"

class FUghSpriteSizes;
class UInstancedStaticMeshComponent;

/**
 * The figures of the level as grey boxes in front of the background: the copters (body, rotor, who rides in them),
 * the passengers with their bubbles, the enemies, the bonus items and the raindrops. Drawn between the views of two
 * steps of the logic (render interpolation); a figure that jumps further than a step can move is not interpolated.
 */
UCLASS()
class AUghFigures : public AActor
{
	GENERATED_BODY()

public:
	AUghFigures();

	/** Shows the figures between `Previous` and `Current` (Alpha 0 .. 1); none outside the play. */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, const FUghSpriteSizes& Sizes);

protected:
	virtual void BeginPlay() override;

private:
	/** The copters; who rides in them or hangs below goes to `OutRiders`. */
	void ShowCopters(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
		TArray<FTransform>& OutRiders);
	void ShowEntities(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
		const FUghSpriteSizes& Sizes, const TArray<FTransform>& Riders);
	void ShowRain(const ugh_logic_view& Current);
	void Clear();

	UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> CopterBodies;   // by player
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rotors;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Passengers;   // also the ones in or below a copter
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bubbles;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Enemies;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> BonusItems;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Raindrops;
};
