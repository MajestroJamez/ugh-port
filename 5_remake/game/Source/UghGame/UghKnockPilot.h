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
