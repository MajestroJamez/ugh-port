// The keys of the players to the inputs of the logic.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class FUghSimulation;

/**
 * The keyboard of the remake (Adapter): a key event of the engine to the inputs of the logic. Pilot 1 flies with
 * the arrows and fires with Right Ctrl or Space (as the original's first pilot); pilot 2 with W A S D and Left Ctrl
 * (the original's Z for down is Y on a Czech keyboard, so S instead). Esc gives the game up, P would pause; every
 * other press and every release is a key the game loop sees (a caption waits for one), as in the original.
 */
class FUghKeyboard
{
public:
	/** Passes a key event to the logic; key repeats are not key events of the original. */
	static void Handle(FUghSimulation& Simulation, const FKey& Key, EInputEvent Event);
	/** The key pilot `Player` (0 or 1) presses for `LogicKey` (UGH_LOGIC_KEY_...); the first one of two. */
	static FKey KeyOf(int32 Player, int32 LogicKey);
};
