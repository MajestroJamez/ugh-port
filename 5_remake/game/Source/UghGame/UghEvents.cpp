#include "UghEvents.h"

#include "UghEffectPlayer.h"
#include "UghSoundPlayer.h"

void UghEvents::Play(TConstArrayView<ugh_logic_event> Events, const ugh_logic_view& Previous,
	const ugh_logic_view& Current, FUghSoundPlayer& Sounds, FUghEffectPlayer& Effects)
{
	for (const ugh_logic_event& Event : Events)
	{
		Sounds.OnEvent(Event);
		Effects.OnEvent(Event, Current);
	}
	Sounds.OnView(Current);
	Effects.OnView(Previous, Current);
}
