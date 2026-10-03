// The game playing itself for a screenshot.
#pragma once

#include "CoreMinimal.h"

class FUghSimulation;

/**
 * -UghShot[=<file.png>] [-UghShotAt=<seconds>]: the game plays level 1 by itself (a check without a window, with
 * -RenderOffscreen): it goes on from the caption, keeps the copter hovering where it is when the level is fully
 * shown (the copter starts in the air and falls), takes the screenshot `At` seconds later and quits; it gives up
 * after TimeLimit.
 */
class FUghShot
{
public:
	/** What the game mode does after a frame. */
	enum class EAction : uint8 { None, TakeShot, Quit };

	/** Reads the command line; false when no shot is asked for. */
	bool Configure();
	const FString& GetPath() const { return Path; }

	/** One frame of the autopilot: its keys go to the logic. */
	EAction Tick(FUghSimulation& Simulation, float DeltaSeconds);

private:
	static constexpr double CaptionKeyEvery = 0.5, QuitAfterShot = 0.6, TimeLimit = 120;

	FString Path;
	double At = 2;
	int32 Phase = -1;        // the phase of the last frame
	double PhaseTime = 0;    // how long it has been in it (in the play: since fully shown)
	double TotalTime = 0;
	int32 HoverY = -1;       // where the copter hovers, 1/32 px
	bool bPedalling = false;
};
