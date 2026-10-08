// The flight over the sea to the stone at the start of a level.
#pragma once

#include "CoreMinimal.h"
#include "UghFlight.h"

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
	 * over frames, the new level's geometry is uploaded and its pipelines compiled: frames much longer than the
	 * others), then it flies: after SettleFrames frames no longer than MaxStep, at most SettleLimit frames.
	 */
	static constexpr int32 SettleFrames = 8, SettleLimit = 30;

	/** From the start. */
	void Start();
	/**
	 * Flies on `Seconds` (at most MaxStep, but hurried; nothing while it settles); it ends at its end.
	 */
	void Advance(double Seconds);
	/**
	 * The rest of the flight in HurrySeconds, its clock speeding up smoothly (from SkipTo before its end when nothing is
	 * seen yet).
	 */
	void Hurry();
	/** No flight (back to the menu). */
	void Stop() { Clock.Stop(); }
	bool IsFlying() const { return Clock.IsRunning(); }
	/** Seconds of the flight so far (Duration at its end). */
	double GetTime() const { return Clock.GetTime(); }
	/** How much of the scene shows, 0 .. 1: from black as it starts. */
	double Shown() const { return FMath::Clamp(Faded / FadeIn, 0.0, 1.0); }

	/** The camera now on the way to `End` (the game's camera) over the sea's surface at the world's `SeaZ`. */
	FUghCameraPose Pose(const FUghCameraPose& End, double SeaZ) const { return At(End, SeaZ, GetTime()); }
	/** The camera `Time` seconds into the flight to `End`: `End` itself from Duration on. */
	static FUghCameraPose At(const FUghCameraPose& End, double SeaZ, double Time);

	/** How its speed changes (UghFlight): at full speed from the start (it starts in black), braking from 0.35 on. */
	static constexpr UghFlight::FProfile Profile{ 0, 0.35 };

	/**
	 * The way to the end over the sea at the world's `SeaZ` (UghFlight::FWay): low over the waves, a swerve to the right
	 * and back, rising at the end; its points and the end's place.
	 */
	static TArray<FVector> Waypoints(const FUghCameraPose& End, double SeaZ);

private:
	FUghFlightClock Clock{ Duration, HurrySeconds, Profile };
	double Faded = 0;   // seconds since it began to show
	int32 Settled = 0;   // short frames held still so far
	int32 Held = 0;      // frames held still so far
};
