// The warning over a copter flying fast enough to crash.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/**
 * Exclamation marks over a copter that flies so fast that hitting what lies ahead would cost a life: the logic's own
 * verdict (ugh_logic_get_copter_danger - the crash limit of the difficulty, the impact a bounce at the copter's speed
 * would have, rock ahead along each axis: not the edges of the screen, which stop a copter without a crash, not the sea,
 * which brakes it), nothing below the limit. ! at the limit, !! from a third of the way from it to the top speed, !!! from
 * two thirds, blinking the faster the louder. Only read: the logic does not change.
 */
namespace UghWarning
{
	/** How loud, 0 (none) .. Loudest. */
	constexpr int32 Loudest = 3;
	/** Blinks a second at each loudness (1 .. 3); the share of a blink it shows. */
	constexpr double BlinkRates[Loudest] = { 2.5, 4, 6 };
	constexpr double BlinkShown = 0.65;
	/** The marks stand this many pixels (the logic's) over the copter's top, over its middle across. */
	constexpr double Above = 7;

	/** How loud the warning of `Danger` is: 0 when the copter would crash into nothing ahead. */
	int32 Loudness(const ugh_logic_copter_danger& Danger);
	/** The loudness of the copter of `Player` in `Logic` now (0: none, no such copter). */
	int32 Of(const ugh_logic* Logic, int32 Player);
	/** Whether a warning of `Loudness` shows `Seconds` into its blinking (always with ugh.Warning.Blink 0: a shot). */
	bool IsLit(int32 Loudness, double Seconds);
}
