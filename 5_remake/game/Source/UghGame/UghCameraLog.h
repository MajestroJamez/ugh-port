// The camera frame by frame, for a look at how smoothly it moves.
#pragma once

#include "CoreMinimal.h"
#include "UghIntro.h"
#include "UObject/UObjectGlobals.h"

/**
 * -UghCameraLog=<file>: the camera of every frame from the start of a flight (to the stone, FUghIntro; between two
 * levels, FUghVoyage, whose camera jumps once in its mist) until
 * AfterSeconds after it ended, written as CSV (when that time is over, or when the game ends): the frame's seconds,
 * the logic's phase, the flight's clock, where the camera is and which way it looks, its lens, exposure and blur, and
 * how fast it moves (units a second of the frame and of the flight's clock - the latter without the frames' jitter),
 * how many textures and meshes wait to stream in.
 * A frame much longer than its neighbours is a hitch; a jump of the speed between two frames a jolt. Only a look:
 * nothing changes.
 */
class FUghCameraLog
{
public:
	/** How long it goes on after the flight (seconds). */
	static constexpr double AfterSeconds = 2.5;

	~FUghCameraLog() { FCoreUObjectDelegates::GetPostGarbageCollect().Remove(Collected); }

	/** On with -UghCameraLog=<file> (-UghCameraLogFlight=<n>: the n-th flight instead of the first). */
	void Configure();
	bool IsOn() const { return !Path.IsEmpty(); }
	/**
	 * The frame `Seconds` after the last one: the camera `Pose`, the logic's `Phase`, a flight (whether one flies - to
	 * the stone, FUghIntro, or between two levels, FUghVoyage -, its clock, its mist). Starts with a flight.
	 */
	void Record(double Seconds, const FUghCameraPose& Pose, int32 Phase, bool bFlying, double FlightTime, double Mist);
	/** Something that happened in this frame (the frame's row says it: a level built, garbage collected). */
	void Note(const FString& What);
	/** Writes what it has (the game ends). */
	void Flush();

private:
	FString Path;
	TArray<FString> Lines;
	bool bRecording = false;
	bool bWritten = false;
	double Since = 0;   // seconds since the flight ended
	double Clock = 0;   // seconds since it began recording
	int32 Frame = 0;
	int32 WantedFlight = 1;   // -UghCameraLogFlight: the flight recorded (from 1)
	int32 Flights = 0;        // begun so far
	bool bWasFlying = false;
	TOptional<FUghCameraPose> Last;
	double LastFlightTime = 0;
	double LastWall = 0;   // FPlatformTime of the last frame
	FString Notes;      // of this frame
	FDelegateHandle Collected;
};
