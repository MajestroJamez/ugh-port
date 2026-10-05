// The events of the logic, heard and seen.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

class FUghEffectPlayer;
class FUghSoundPlayer;

namespace UghEvents
{
	/**
	 * Gives every event of the logic's last steps to the sounds and to the effects in one loop, then the frame's views:
	 * an event plays its sound (FUghSounds::Cues) and shows its burst (FUghEffectPlayer::Cues) at once, from the same
	 * event. Test Ugh.Effects.Events (UghEffectTests.cpp).
	 */
	void Play(TConstArrayView<ugh_logic_event> Events, const ugh_logic_view& Previous, const ugh_logic_view& Current,
		FUghSoundPlayer& Sounds, FUghEffectPlayer& Effects);
}
