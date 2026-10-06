// The flight over the sea to the stone at the start of a level.
#pragma once

#include "CoreMinimal.h"

/** Where the camera is (the world), which way it looks and how wide (degrees across, the engine's field of view). */
struct FUghCameraPose
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	float FieldOfView = 90;
	/** How much what moves across the view blurs (the engine's motion blur amount); negative: the engine's own. */
	float MotionBlur = -1;
	/** The exposure changed by this much (EV; 0 the mood's own, AUghStage). */
	float ExposureBias = 0;
};

/**
 * The start of a level (while its caption shows): the camera flies like a drone low over the open sea towards the
 * stone the level is carved into (AUghSeaStack) - along a smooth curve, banking into its turns, up and down a little,
 * its lens wide, then narrowing - and brakes softly to a stop in the game's camera (AUghStage::Fit), never into the
 * stone: no jolt on the way (its curve bends smoothly, its speed and deceleration change smoothly) and none at the end
 * (speed and deceleration fade to 0). A key hurries it to its end (HurrySeconds: before the play can begin) on a clock
 * that speeds up smoothly from its pace; before anything is seen it starts nearer instead (SkipTo). Only the
 * frontend's: the logic and its timing do not change (the caption waits for its key meanwhile).
 * -UghNoIntro (or bFlies false): no flight.
 */
class FUghIntro
{
public:
	/** Whether a level starts with the flight. */
	static constexpr bool bFlies = true;
	/** Seconds: the flight; its rest when a key hurries it; the scene showing from black as it starts. */
	static constexpr double Duration = 4.5, HurrySeconds = 0.9, FadeIn = 0.3;
	/** Hurried before anything is seen: the flight goes on from this many seconds before its end. */
	static constexpr double SkipTo = 1.6;
	/** A frame longer than this (the level being built) moves the flight only this far, seconds. */
	static constexpr double MaxStep = 1.0 / 20;
	/** The lens at the start (degrees across), how far the camera banks at most (degrees). */
	static constexpr float StartFieldOfView = 72, MaxRoll = 10;
	/**
	 * Its first frames hold still in black: the renderer settles on the new view (its reflections, its light build up
	 * over frames), then it flies.
	 */
	static constexpr int32 SettleFrames = 8;

	/** From the start. */
	void Start();
	/**
	 * Flies on `Seconds` (at most MaxStep, but hurried; nothing the first SettleFrames frames); it ends at its end.
	 */
	void Advance(double Seconds);
	/**
	 * The rest of the flight in HurrySeconds, its clock speeding up smoothly (from SkipTo before its end when nothing is
	 * seen yet).
	 */
	void Hurry();
	/** No flight (back to the menu). */
	void Stop() { bFlying = false; }
	bool IsFlying() const { return bFlying; }
	/** Seconds of the flight so far (Duration at its end). */
	double GetTime() const { return Time; }
	/** How much of the scene shows, 0 .. 1: from black as it starts. */
	double Shown() const { return FMath::Clamp(Faded / FadeIn, 0.0, 1.0); }

	/** The camera now on the way to `End` (the game's camera) over the sea's surface at the world's `SeaZ`. */
	FUghCameraPose Pose(const FUghCameraPose& End, double SeaZ) const { return At(End, SeaZ, Time); }
	/** The camera `Time` seconds into the flight to `End`: `End` itself from Duration on. */
	static FUghCameraPose At(const FUghCameraPose& End, double SeaZ, double Time);

private:
	double Time = 0;
	double Faded = 0;   // seconds since it began to show
	bool bHurried = false;
	double Hurried = 0, HurryFrom = 0;   // seconds since the key, the flight's time then
	/** The rest of the way after the key: the coefficients of its quintic (bHurryAlong), else of its clock (Hurry). */
	double Hurry0 = 0, Hurry1 = 0, Hurry2 = 0, Hurry3 = 0, Hurry4 = 0, Hurry5 = 0;
	bool bHurryAlong = false;
	int32 Settled = 0;   // frames held still so far
	bool bFlying = false;
};
