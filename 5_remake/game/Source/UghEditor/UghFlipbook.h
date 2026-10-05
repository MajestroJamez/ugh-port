// A flame's frames laid out as a flipbook.
#pragma once

#include "CoreMinimal.h"

/** The frames of a simulated flame (AUghFireBake) made into the flipbook of UghFlames. */
namespace UghFlipbook
{
	/** A picture of linear light, row by row. */
	struct FFrame
	{
		int32 Width = 0, Height = 0;
		TArray<FLinearColor> Pixels;
	};

	/**
	 * The flipbook (UghFlames: Columns x Rows cells of CellWidth x CellHeight, row by row, sRGB) of `Simulated` (at least
	 * UghFlames::Frames + Blend of the same size, in the order of the simulation): its brightest light (a high
	 * percentile, not a lone spark) white, cropped around where it burns in any frame (its foot at the bottom of the
	 * cells, as much room on its sides), the last Blend frames blended into the first ones so that it loops; empty
	 * when there are too few frames or nothing burns.
	 */
	TArray<FColor> Make(const TArray<FFrame>& Simulated);

	/**
	 * Where `Frames` (all of one size) burn on the whole: their light added up is more than `Threshold` of its brightest
	 * (pixels; empty: nowhere).
	 */
	FBox2D Burning(const TArray<FFrame>& Frames, float Threshold);
}
