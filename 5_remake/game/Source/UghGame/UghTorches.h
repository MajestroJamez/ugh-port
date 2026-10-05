// The torches on the cave's walls.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghDecorations.h"
#include "UghTorches.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * The torches of the level being played (UghDecorations, kind Torch): the torch of Blender/torch.py (UghAssets::Torch:
 * a crooked shaft, its head of bark and pitch glowing with embers, UghMaterials::Embers; a clay shaft and head without
 * it) wedged into the rock, leaning out of the wall and a little aside, a small flame baked from a simulated fire on
 * its head (UghMaterials::Flame, the flipbook UghFlames::Torch), a few sparks and a wisp of smoke (UghFireParts) and a
 * flickering light that lights the wall around it, brighter in the darker moods. A torch goes out when the water
 * rises over it. Only decoration: the game does not know them.
 */
UCLASS()
class AUghTorches : public AActor
{
	GENERATED_BODY()

public:
	AUghTorches();
	virtual void Tick(float DeltaSeconds) override;

	/** Lights the torches among `Decorations`, their flames leaning with `Wind`, `FireLight` times as bright as by day. */
	void Place(const TArray<FUghDecoration>& Decorations, int32 Wind, float FireLight);
	/** The water surface (pixels from the top): a torch burns only while the water is below its head. */
	void SetWater(double Surface);

	/** Where the head of `Torch`'s shaft is (the world): its flame burns there. */
	static FVector Head(const FUghDecoration& Torch);

protected:
	virtual void BeginPlay() override;

private:
	void Show();
	void Flare();

	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Shafts;   // the model, or a clay shaft
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ClayHeads;   // without the model
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Flames;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Sparks;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Smoke;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SparkMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SmokeMaterial;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lights;
	TArray<FUghDecoration> Torches;
	TArray<bool> Burning;
	double ModelScale = 1;   // the model's, to be ShaftLength long
	float Flicker = 0;
	float Candelas = 0;
	double Time = 0;
};
