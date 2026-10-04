// The outline of the rock of a level in the plane of the play.
#pragma once

#include "CoreMinimal.h"
#include "UghShapes.h"

struct ugh_logic;

/**
 * The collision mask of the level being played as distances, on a grid of the pixels' centres that reaches Margin
 * pixels beyond the screen's edges (there each pixel is the nearest one of the screen): how far each centre is from
 * the edge between rock and air, positive in the rock. Two pixels side by side of different kinds are both half a
 * pixel from it, so the edge lies exactly between them, on the pixels' border (FUghRockField). Also the distances
 * blurred, for the shapes behind the slab of the play that need not follow the pixels.
 */
class FUghRockOutline
{
public:
	/** How far the grid reaches beyond the screen at the left and the right, at the top and the bottom (pixels). */
	static constexpr int32 MarginX = 48, MarginY = 24;
	/** The size of the grid. */
	static constexpr int32 Width = UghShapes::ScreenWidth + 2 * MarginX, Height = UghShapes::ScreenHeight + 2 * MarginY;

	/** The centre of grid column I / row J in pixels of the screen. */
	static double X(int32 I) { return I - MarginX + 0.5; }
	static double Y(int32 J) { return J - MarginY + 0.5; }

	/** Reads the mask of the level being played (none loaded: all air). */
	void Build(const ugh_logic* Logic);

	bool Solid(int32 I, int32 J) const { return Distances[Index(I, J)] > 0; }
	/** Signed: from the centre to the edge between rock and air, pixels (at most MaxDistance). */
	float Distance(int32 I, int32 J) const { return Distances[Index(I, J)]; }
	/** Distance blurred (a few pixels), and which way it grows (pixels per pixel: x right, y down). */
	float Soft(int32 I, int32 J) const { return Blurred[Index(I, J)]; }
	FVector2f SoftGradient(int32 I, int32 J) const { return Gradients[Index(I, J)]; }
	/** How far the centre of I, J is outside the screen, pixels (0 on it). */
	static double Outside(int32 I, int32 J);

	static int32 Index(int32 I, int32 J) { return J * Width + I; }

private:
	static constexpr float MaxDistance = 64;
	/** The blur of Soft: a Gaussian of this many pixels. */
	static constexpr float BlurSigma = 1.5f;

	void Blur();

	TArray<float> Distances;
	TArray<float> Blurred;
	TArray<FVector2f> Gradients;
};
