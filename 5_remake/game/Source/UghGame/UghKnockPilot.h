// The autopilot that knocks a passenger off its pad (a shot of the flung passengers and their test).
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/** The keys of a pilot the knock pilot holds (UGH_LOGIC_KEY_UP, LEFT, RIGHT). */
struct FUghPilotKeys
{
	bool bUp = false, bLeft = false, bRight = false;
};

/**
 * Flies copter 0 into the first passenger waiting or walking on land (out of its door), with its body Clearance
 * pixels above the passenger's pad - into the passenger, not landing (a copter landed on its pad would take it on) -,
 * so that it knocks it into the water (the logic's OnPickupPad): up or down to that height first, then across to it;
 * once `bKnocked` it hovers where it is. Only for a shot (FUghShot -UghShotFling) and the test Ugh.Fling: no player
 * flies so.
 */
class FUghKnockPilot
{
public:
	/** Its body's bottom this many pixels above the pad. */
	static constexpr double Clearance = 5;
	/** It goes across only within this many pixels of that height. */
	static constexpr double Level = 4;

	/** The keys to hold after `Current` (the view of the step before: `Previous`) of `Logic`. */
	static FUghPilotKeys Fly(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current,
		bool bKnocked);
};

/**
 * Flies copter 0 over open water and lets it fall into the sea (a shot of its splash and its test, FUghDunks): plans
 * once where - the highest place (the biggest splash; the nearest of those) with nothing solid from there down to Below
 * pixels under the surface (its body and Margin either side) and the way there clear -, goes up or down to that
 * height, across to it and lets go: it falls by itself, steering only to stay above the place (the keys across do not
 * hold it up). Only for a shot (FUghShot -UghShotDunk) and the test Ugh.Dunk: no player flies so.
 */
class FUghDunkPilot
{
public:
	/** Clear this many pixels either side of its body, this far under the surface (it dives about 20 px). */
	static constexpr int32 Margin = 6, Below = 30;
	/** A fall shorter than this (pixels to the water) is no splash worth a look: another column. */
	static constexpr int32 LeastFall = 40;
	/** It lets go within this many pixels of the place (falling it steers on to stay above it). */
	static constexpr double Near = 2;

	/** The keys to hold after `Current` (the view of the step before: `Previous`) of `Logic`. */
	FUghPilotKeys Fly(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current);
	/** It has let go: the copter falls. */
	bool HasDropped() const { return bDropped; }
	/** The place it lets go at (its corner, pixels), none before it planned or when there is none. */
	TOptional<FIntPoint> GetPlace() const { return Place; }
	/** Seconds until copter 0's waterline reaches the surface falling freely, none when it is not falling above it. */
	static TOptional<double> UntilSplash(const ugh_logic_view& Previous, const ugh_logic_view& Current);

private:
	void Plan(const ugh_logic* Logic, const ugh_logic_view& View);

	bool bPlanned = false, bDropped = false;
	TOptional<FIntPoint> Place;
};
