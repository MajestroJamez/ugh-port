// Where a figure's action is between the frames of its sprite's animation.
#pragma once

#include "CoreMinimal.h"

/**
 * The logic says only which frame of an animation a figure shows; its model's action goes on smoothly instead, as
 * fast as the frames have been changing lately (smoothed over SmoothSeconds), and stays within the frame shown: a
 * new frame starts the action at that frame's part of the loop (Phase = frame / frames), and it never runs past the
 * frame's end before the next one comes.
 */
class FUghFrameClock
{
public:
	static constexpr double SmoothSeconds = 0.3;
	/** Frames a second before any have been seen to change (a passenger walks with a frame every 7 steps). */
	static constexpr double FirstFramesPerSecond = 10;

	/** A frame `Seconds` after the last one, the sprite showing frame `Frame` of `Frames`. */
	void Update(int32 Frame, int32 Frames, double Seconds);

	/** Where the action is in its loop: 0 .. 1. */
	double Phase() const { return Position; }

private:
	int32 LastFrame = -1;
	double Since = 0;   // seconds since the last frame began
	double FramesPerSecond = FirstFramesPerSecond;
	double Position = 0;
};
