// The campfires in the cave.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UghDecorations.h"
#include "UghHearths.h"
#include "UghCampfire.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;

/**
 * The campfires of the level being played (UghDecorations, kind Campfire): a hearth of stones, charred logs and glowing
 * coals (FUghHearths), a flame baked from a simulated fire playing on crossed cards (UghMaterials::Flame, the flipbook
 * UghFlames::Campfire), sparks shooting up out of it and a thin plume of smoke (UghFireParts), and a light that
 * flickers in brightness, colour and place (Lumen colours the rock warm, the shadows dance), brighter in the darker
 * moods; each fire its own way; in the wind the flames lean and flicker more, the sparks and smoke drift. A fire goes
 * out when the water rises over its ledge. Only decoration: the game does not know them.
 */
UCLASS()
class AUghCampfire : public AActor
{
	GENERATED_BODY()

public:
	AUghCampfire();
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Lights the campfires among `Decorations` (the others are not this actor's), leaning with the level's `Wind`, their
	 * lights `FireLight` times as bright as by day (the mood's).
	 */
	void Place(const TArray<FUghDecoration>& Decorations, int32 Wind, float FireLight);
	/** The water surface (pixels from the top): a fire burns only while the water is below its ledge. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	/** Shows the fires that burn: their hearths, flames, sparks, smoke and lights. */
	void Show();
	/** The lights of the fires that burn as they flicker now. */
	void Flare();

	FUghHearths Hearths;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Flames;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Sparks;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Smoke;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> FlameMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SparkMaterial;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SmokeMaterial;
	UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lights;   // made as many as needed, hidden when unused
	TArray<FUghDecoration> Fires;
	TArray<bool> Burning;   // of Fires
	TArray<FVector> LightPlaces;   // of Fires: where its light is when it does not flicker
	float Flicker = 0;      // how much the lights flicker
	float Candelas = 0;
	int32 Wind = 0;
	double Time = 0;
};
