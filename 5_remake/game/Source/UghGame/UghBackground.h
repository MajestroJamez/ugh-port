// The level around the figures.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghBackground.generated.h"

class UInstancedStaticMeshComponent;
class UTextRenderComponent;
struct ugh_logic;

/**
 * The background of the level being played as grey boxes: its solid pixels (the collision mask, the plane of the
 * play exactly), its pads with their numbers, the water and a wall behind the screen.
 */
UCLASS()
class AUghBackground : public AActor
{
	GENERATED_BODY()

public:
	/** The depth of the background around the plane of the play (units): the figures are in front of it. */
	static constexpr double Thickness = 400.0;

	AUghBackground();

	/** Builds the level being played; nothing before the first one is loaded. */
	void Build(const ugh_logic* Logic);
	/** The water surface, pixels from the top of the screen. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	void BuildRock(const ugh_logic* Logic);
	void BuildPads(const ugh_logic* Logic);

	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Wall;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rock;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Pads;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Water;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> PadNumbers;
};
