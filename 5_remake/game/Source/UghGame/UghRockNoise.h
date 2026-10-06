// The fractured blocks the rock is shaped with, and how its shapes meet.
#pragma once

#include "CoreMinimal.h"

/** Noise of fractured limestone (blocks with flat faces, cracks between them; pixels of the screen), smooth unions. */
namespace UghRockNoise
{
	struct FBlock
	{
		/** How far its block stands out, 0 .. 1: each block its own, its face flat and tilted a little. */
		double Height = 0;
		/** 0 on the crack between two blocks (where there is one), 1 within a block. */
		double Crack = 1;
	};

	/**
	 * The block at x, y: cells of a jittered grid about `Width` x `Height` pixels (wider than high: ledges), their
	 * borders a little wavy; `Salt` makes another pattern.
	 */
	FBlock Blocks(double X, double Y, double Width, double Height, uint32 Salt);

	/**
	 * How steeply the strata of the stone's slate rise to the right (y up a pixel of x): the beds of the level's rock,
	 * of the sea stack and of the cliff's material (UghCliff.hlsl: the same dip) lie alike, one boulder.
	 */
	constexpr double StrataDip = 0.14;

	/**
	 * The slate at x, y (pixels, y down): beds about `BedHeight` thick (some thicker, some thinner, undulating) rising
	 * to the right by StrataDip, each broken into flakes about `FlakeLength` long along `Along` (pixels along the
	 * strata: x on the face, x and the depth around the stone). Height 0 .. 1: a flake stands out most at its foot (a
	 * sharp lip over the bed below, which starts deep under it: a shadow), each flake its own way; Crack 0 in the open
	 * seam between two flakes of a bed. `Salt` makes another pattern.
	 */
	FBlock Slate(double X, double Y, double Along, double BedHeight, double FlakeLength, uint32 Salt);

	/** The greater of two fields, rounded where they are within `K` of each other (two shapes melting together). */
	double SmoothMax(double A, double B, double K);
}
