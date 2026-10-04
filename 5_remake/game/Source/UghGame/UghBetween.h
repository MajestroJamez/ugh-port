// Where a figure is between two steps of the logic.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/**
 * Render interpolation: a frame is drawn between the views of the last two steps of the logic (Alpha 0 .. 1). A
 * figure that moved further in one step than it can move jumped (a new attempt, a passenger getting in): it is not
 * interpolated.
 */
namespace UghBetween
{
	/** A figure that moves further in one step jumped. */
	constexpr double MaxStepPixels = 8;

	/** A position of the view (1/32 px) in pixels. */
	FVector2D Pixels(int32 X, int32 Y);
	/** A position (1/32 px) between two steps, in pixels; the later one when it jumped. */
	FVector2D Position(int32 X0, int32 Y0, int32 X1, int32 Y1, double Alpha);
	/** True when a figure moved from X0, Y0 to X1, Y1 (1/32 px) in one step without jumping. */
	bool Moved(int32 X0, int32 Y0, int32 X1, int32 Y1);
	/** The view to interpolate from: `Previous`, unless `Current` is of a new level or attempt (then itself). */
	const ugh_logic_view& From(const ugh_logic_view& Previous, const ugh_logic_view& Current);
}
