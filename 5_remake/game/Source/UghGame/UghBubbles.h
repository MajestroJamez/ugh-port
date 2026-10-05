// The passengers' speech bubbles.
#pragma once

#include "CoreMinimal.h"
#include "UghShapes.h"

struct ugh_logic;

/** What a speech bubble shows: the board of the pad a passenger wants to go to (its tally marks), or a question. */
struct FUghBubbleLook
{
	int32 Marks = 0;          // UghPadSigns::Marks of the pad's number: none on a blank board
	bool bQuestion = false;   // a question mark instead (the copter left without it)

	bool operator==(const FUghBubbleLook& Other) const = default;
};

/** Where a bubble is: its top left corner (pixels), and on which side its one tail is (it points at its passenger). */
struct FUghBubblePlace
{
	FVector2D At = FVector2D::ZeroVector;
	bool bTailLeft = true;

	/** Pixels: the tip of its tail. */
	FVector2D Tip() const;
};

/**
 * The speech bubbles as the original shows them, drawn sharp: a round white bubble as big as the original's sprite
 * (16 x 15 px) with one tail at its bottom pointing down at its passenger's head, holding the board with the number
 * of the pad the passenger wants to go to (pale tally marks cut into brown wood, as the pads' boards) or a question
 * mark. The original draws the bubble 11 px right of its passenger and 13 px above it, its tail on the left
 * (the frame of 113b:0ca5, Frame.kt of the Kotlin port; no sprite has a tail on the right); a bubble that would leave
 * the screen there is mirrored to the passenger's left, its tail on the right, so it still points at the passenger.
 */
namespace UghBubbles
{
	/** Pixels: a bubble is as big as the original's sprite, this far from its passenger's top left corner. */
	constexpr double Width = 16, Height = 15;
	constexpr double OffsetX = 11, OffsetY = -13;
	/** A bubble's picture has this many texels a pixel: about a texel a screen pixel at 1080p (192 px high). */
	constexpr int32 Scale = 8;
	constexpr int32 PictureWidth = int32(Width) * Scale, PictureHeight = int32(Height) * Scale;

	/** What bubble `Sprite` (ugh_logic_entity.bubble) shows, by its name in the data; unset for another sprite. */
	TOptional<FUghBubbleLook> Look(const ugh_logic* Logic, int32 Sprite);
	/** Where the bubble of a passenger whose sprite's top left corner is at `Passenger`, `PassengerWidth` wide, is. */
	FUghBubblePlace Place(const FVector2D& Passenger, double PassengerWidth);
	/** The picture of a bubble: PictureWidth x PictureHeight texels row by row, sRGB, transparent around it. */
	TArray<FColor> Draw(const FUghBubbleLook& Look, bool bTailLeft);
}
