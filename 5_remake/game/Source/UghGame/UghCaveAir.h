// The air of the cave: haze, shafts of light, the fires' glow.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghDecorations.h"
#include "UghCaveAir.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UTexture2D;
struct FUghMood;

/** A fire whose glow lies in the cave's air (a campfire, a torch): pixels of the screen. */
struct FUghAirFire
{
	FVector2D At;
	/** How far its glow reaches through the air (pixels, the way around rock), how bright it is at the fire. */
	double Reach = 0;
	float Strength = 0;
};

/** The map of the cave's air over the screen, computed from the collision mask (AUghCaveAir). */
namespace UghCaveAir
{
	/** The map's cells: Scale x Scale pixels of the screen each. */
	constexpr int32 Scale = 2;
	constexpr int32 Width = UGH_LOGIC_SCREEN_WIDTH / Scale, Height = UGH_LOGIC_SCREEN_HEIGHT / Scale;
	/** Pixels: how far up a rock shelters the air below it; how far a shaft of light reaches into the cave. */
	constexpr double ShelterReach = 70, ShaftReach = 150;

	/** The glows of the fires of `Decorations` (campfires and torches) above the water's surface (`WaterRow`). */
	TArray<FUghAirFire> Fires(TConstArrayView<FUghDecoration> Decorations, double WaterRow);
	/**
	 * The map of the air of `Logic`'s level, Width x Height cells row by row: r how sheltered the air is (rock above it
	 * and beside above, near: 1), g how much a shaft of light going `Way` (pixels on the screen: x right, y down) lights
	 * it - a cell whose way back against the light leaves the screen through air within ShaftReach, the more the nearer
	 * -, b the glow of `Fires` (fading along the way through the air, around rock), a how much of the cell is air (0 in
	 * rock). All 0 .. 1; none before a level.
	 */
	TArray<FLinearColor> Map(const ugh_logic* Logic, const FVector2D& Way, TConstArrayView<FUghAirFire> Fires);
}

/**
 * The cave's air (step 30c), so that the caves are no black holes: a card across the screen just behind the figures'
 * room (M_UghCaveAir) with a haze in it the thicker the deeper the cave behind - the back wall far behind hazy, the
 * walls near it clear -, hazier where rock shelters it (the dark corners read as space, not as black), shafts of the
 * sun (the moon at night) streaking down through the air the light reaches, the fires' warm glow spreading through the
 * air around them, deeper into the cave than their lights reach (a fire drowned by the rising water glows no more).
 * Cheap at every preset (one card, one texture, no lights, no shadows): the figures stand in front of it, the haze as
 * dim as the shade's light (FUghMood::Shade), never brighter than them.
 */
UCLASS()
class AUghCaveAir : public AActor
{
	GENERATED_BODY()

public:
	AUghCaveAir();

	/** The air of `Logic`'s level (none before a level) with its fires (`Decorations`), in `Mood` with the light going `Sun`. */
	void Build(const ugh_logic* Logic, TConstArrayView<FUghDecoration> Decorations, const FUghMood& Mood,
		const FVector& Sun, double WaterRow);
	/** The water's surface (pixels from the top): the air above it, the fires it drowned glowing no more. */
	void SetWater(double Surface);

protected:
	virtual void BeginPlay() override;

private:
	void ShowMap();

	const ugh_logic* Logic = nullptr;
	TArray<FUghDecoration> Decorations;
	FVector2D Way = FVector2D(0, 1);
	double Surface = 0;
	int32 FiresLit = -1;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Card;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Material;
	UPROPERTY() TObjectPtr<UTexture2D> MapTexture;
};
