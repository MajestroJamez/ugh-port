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

/** The keys of a pilot the drop pilot holds: those of FUghPilotKeys and fire. */
struct FUghDropKeys : FUghPilotKeys
{
	bool bFire = false;
};

/**
 * Flies copter 0 to the stone (the standing passenger), takes it on its sling, flies above the first enemy and lets
 * it go, so that it falls onto the enemy and bounces off it (a shot of the stone falling and hitting): the ways there
 * found on the collision mask (the copter's outline with Margin pixels around it, a breadth-first search over its
 * corner's places), followed slowly a few pixels ahead; once it let go it hovers where it is. Only for a shot
 * (FUghShot -UghShotDrop): no player flies so.
 */
class FUghDropPilot
{
public:
	/** Free this many pixels around the copter's outline on its way. */
	static constexpr int32 Margin = 2;
	/** Its skids this many pixels into the stone's top take it (the logic's touch box of the stone is lower than its
	 * sprite); it lets go this many pixels above the enemy's top. */
	static constexpr int32 Into = 8, Above = 34;
	/** It follows its way this many places ahead; it lets go within Near pixels of the place, nearly still. */
	static constexpr int32 Ahead = 5;
	static constexpr double Near = 1.5, Still = 0.12;

	/** The keys to hold after `Current` (the view of the step before: `Previous`) of `Logic`. */
	FUghDropKeys Fly(const ugh_logic* Logic, const ugh_logic_view& Previous, const ugh_logic_view& Current);
	/** It has let the stone go (the logic took it off the sling). */
	bool HasDropped() const { return bDropped; }
	/** No stone or no enemy in the level, or no way to them. */
	bool IsLost() const { return bLost; }

private:
	/** The way from `From` to `To` (the copter's corner, pixels), from the next place on; empty when there is none. */
	static TArray<FIntPoint> Way(const ugh_logic* Logic, const FIntPoint& From, const FIntPoint& To);

	enum class EStage : uint8 { ToStone, ToEnemy, Dropped };
	EStage Stage = EStage::ToStone;
	TArray<FIntPoint> Path;
	int32 Along = 0;
	bool bDropped = false, bLost = false, bFired = false;
	TOptional<FIntPoint> Enemy;   // the corner of the enemy to drop it onto
};
