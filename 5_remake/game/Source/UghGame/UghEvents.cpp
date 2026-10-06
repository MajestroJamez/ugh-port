#include "UghEvents.h"

#include "UghEffectPlayer.h"
#include "UghFling.h"
#include "UghSoundPlayer.h"

void UghEvents::Play(TConstArrayView<ugh_logic_event> Events, const ugh_logic_view& Previous,
	const ugh_logic_view& Current, FUghSoundPlayer& Sounds, FUghEffectPlayer& Effects, FUghFlings* Flings)
{
	for (const ugh_logic_event& Event : Events)
	{
		Sounds.OnEvent(Event);
		if (Flings)
		{
			Flings->OnEvent(Event, Current);
		}
		Effects.OnEvent(Event, Current);
	}
	Sounds.OnView(Current);
	if (Flings)
	{
		Flings->OnView(Previous, Current);
	}
	Effects.OnView(Previous, Current);
}
