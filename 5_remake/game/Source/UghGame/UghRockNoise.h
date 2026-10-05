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

	/** The greater of two fields, rounded where they are within `K` of each other (two shapes melting together). */
	double SmoothMax(double A, double B, double K);
}
