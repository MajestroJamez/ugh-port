// The soft shadows under the copters.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ugh_logic.h"
#include "UghCopterShadows.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;

/** Where a copter's shadow lies (pixels of the screen) and how it looks. */
struct FUghCopterShadow
{
	/** The middle of its body across, the surface's row the shadow lies on, how high its body's bottom is above it. */
	double X = 0, Surface = 0, Height = 0;
	/** How dark (0 .. Darkest) and how wide (half, pixels): darker and tighter the lower the copter. */
	double Opacity = 0, HalfWidth = 0;
};

namespace UghCopterShadow
{
	/** Pixels above the surface by which the shadow has faded out; its opacity right on it. */
	constexpr double FadeHeight = 80, Darkest = 0.55;
	/** Pixels: how far it reaches in front of and behind the plane of the play, above and below its surface. */
	constexpr double HalfDepth = 9, Above = 1.5, Below = 1;

	/**
	 * The shadow of a copter whose body is `Body` (pixels) in the level of `Logic`: on the first surface of the collision
	 * mask under its bottom - the highest first solid pixel of the columns under the middle half of its body -; none
	 * when that is under the water's surface (`WaterRow`) or off the screen, or the body is FadeHeight or more above it.
	 */
	TOptional<FUghCopterShadow> Under(const ugh_logic* Logic, const FBox2D& Body, double WaterRow);
	/** The body of a copter at pixel x, y (its sprite's corner). */
	FBox2D BodyAt(const FVector2D& Corner);
	/**
	 * The world transform of the decal of `Shadow`: projecting down onto the surface, its unit box scaled to Below ..
	 * Above about the surface, HalfDepth across the depths, HalfWidth across the screen.
	 */
	FTransform Place(const FUghCopterShadow& Shadow);
}

/**
 * The soft shadows under the copters (step 30e): a dark blob on the surface below each copter (UghCopterShadow::Under)
 * - the pad or the rock it would land on -, darker and tighter the lower it flies, faded out FadeHeight above it, so
 * the height is readable and a landing easier. A deferred decal (M_UghCopterShadow) on whatever lies there (the rock,
 * the turf, the feet of people standing there; not the copters), at every quality preset: the sun's shadow of a copter
 * (where the preset has one) falls behind it onto the cave, away to the side - this one is the light of the sky kept
 * off what is under it, no second sun shadow. Not on the ghost.
 */
UCLASS()
class AUghCopterShadows : public AActor
{
	GENERATED_BODY()

public:
	AUghCopterShadows();

	/** Shows the shadows of the copters between `Previous` and `Current` (Alpha) in the level of `Logic`; none outside the play. */
	void Show(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha);

private:
	UPROPERTY() TArray<TObjectPtr<UDecalComponent>> Decals;   // by player
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
};
