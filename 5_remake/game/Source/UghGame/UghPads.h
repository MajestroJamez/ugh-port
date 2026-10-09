// The gamepads' motors.
#pragma once

#include "CoreMinimal.h"
#include "UghImpacts.h"

/**
 * The rumble of the pilots' gamepads (FUghImpacts::Rumble), each pad its own: pilot 1's is the first gamepad
 * connected, pilot 2's the second, as FUghControls gives them their pilots (the engine numbers the pads in the order
 * XInput connects them). The engine's own force feedback cannot tell two pads apart - every gamepad is the one local
 * player's (Config/Windows/WindowsInput.ini), so it would rumble the last one used -, so the motors are set through
 * XInput directly, only when they change. Nothing on a computer without XInput or without the pad.
 */
class FUghPads
{
public:
	~FUghPads() { StopAll(); }

	/** Pilot `Pilot`'s pad's motors now. */
	void Set(int32 Pilot, const FUghRumble& Rumble);
	/** Every motor still. */
	void StopAll();

private:
	/** The XInput user index of pilot `Pilot`'s pad, none without it. */
	int32 PadOf(int32 Pilot);

	FUghRumble Sent[FUghImpacts::Pilots];
	TArray<int32> Connected;   // the XInput user indices connected, looked at most once a second
	double LookedAt = -1e9;
};
