// How much what moves blurs in the play.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/**
 * The motion blur of the play: the engine's own, per pixel from the velocity buffer (it works before the upscalers -
 * TSR, FSR, DLSS - take the frame, which want the same velocities), so with the game's camera standing still only what
 * moves blurs: the copters with their pilots, riders and the stone in the sling, the rotors and cranks turning (their
 * own rotational blur), a little the people walking and the flung. How much (the engine's motion blur amount: the
 * shutter as a part of a frame at its 30 fps) grows with the speed of the fastest copter in the logic, the pixels it
 * went in the last step: Rest up to SlowPixels (a hovering copter's rotor turning, a slow copter sharp - its blur is
 * as long as its speed times the amount), up to Full at FastPixels (smootherstep; the logic's top speed is 3 px a
 * step), following within FollowSeconds; none in a frame a copter jumped (a new attempt: no smear across the screen).
 * The flights to the stone and over the archipelago keep their own (FUghIntro, FUghIsles: the camera flies there).
 * Low has none (the engine's scalability: r.MotionBlurQuality 0), medium a half-resolution gather.
 */
class FUghMotionBlur
{
public:
	static constexpr double Rest = 0.3, Full = 0.75;
	/** Pixels a step of the logic (its top speed: 3 straight, 4.2 diagonally). */
	static constexpr double SlowPixels = 1.0, FastPixels = 2.75;
	static constexpr double FollowSeconds = 0.1;

	/** A frame `Seconds` after the last one, the logic's last two steps' views. */
	void Update(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Seconds);
	/** The amount now (the engine's MotionBlurAmount). */
	float GetAmount() const { return bJumped ? 0.f : float(Amount); }

	/** The pixels the fastest copter went from `Previous` to `Current`; none outside the play; -1 when one jumped. */
	static double Speed(const ugh_logic_view& Previous, const ugh_logic_view& Current);
	/** The amount at a copter's speed (pixels a step). */
	static double AmountAt(double Speed);

private:
	double Amount = Rest;
	bool bJumped = false;
};
