#include "UghKeyboard.h"

#include "UghSimulation.h"

namespace
{
	/** A key a pilot flies with. */
	struct FPilotKey
	{
		FKey Key;
		int32 Player;
		int32 LogicKey;   // UGH_LOGIC_KEY_...
	};

	/** The keys of both pilots; the first one of a logic key is the one FUghKeyboard::KeyOf gives. */
	const TArray<FPilotKey>& PilotKeys()
	{
		static const TArray<FPilotKey> Keys = {
			{ EKeys::Up, 0, UGH_LOGIC_KEY_UP }, { EKeys::Down, 0, UGH_LOGIC_KEY_DOWN },
			{ EKeys::Left, 0, UGH_LOGIC_KEY_LEFT }, { EKeys::Right, 0, UGH_LOGIC_KEY_RIGHT },
			{ EKeys::RightControl, 0, UGH_LOGIC_KEY_FIRE }, { EKeys::SpaceBar, 0, UGH_LOGIC_KEY_FIRE },
			{ EKeys::W, 1, UGH_LOGIC_KEY_UP }, { EKeys::S, 1, UGH_LOGIC_KEY_DOWN },
			{ EKeys::A, 1, UGH_LOGIC_KEY_LEFT }, { EKeys::D, 1, UGH_LOGIC_KEY_RIGHT },
			{ EKeys::LeftControl, 1, UGH_LOGIC_KEY_FIRE },
		};
		return Keys;
	}

	const FPilotKey* FindPilotKey(const FKey& Key)
	{
		return PilotKeys().FindByPredicate([&](const FPilotKey& PilotKey) { return PilotKey.Key == Key; });
	}
}

FKey FUghKeyboard::KeyOf(int32 Player, int32 LogicKey)
{
	const FPilotKey* Found = PilotKeys().FindByPredicate(
		[&](const FPilotKey& PilotKey) { return PilotKey.Player == Player && PilotKey.LogicKey == LogicKey; });
	return Found ? Found->Key : FKey();
}

void FUghKeyboard::Handle(FUghSimulation& Simulation, const FKey& Key, EInputEvent Event)
{
	if (!Key.IsValid() || Key.IsMouseButton() || Key.IsGamepadKey() || (Event != IE_Pressed && Event != IE_Released))
	{
		return;
	}
	const bool bPressed = Event == IE_Pressed;
	if (const FPilotKey* PilotKey = FindPilotKey(Key))
	{
		Simulation.Key(PilotKey->Player, PilotKey->LogicKey, bPressed);
	}
	if (bPressed && Key == EKeys::Escape)
	{
		Simulation.MenuKey(UGH_LOGIC_MENU_ESCAPE);
	}
	else if (bPressed && Key == EKeys::P)
	{
		Simulation.MenuKey(UGH_LOGIC_MENU_PAUSE);
	}
	else
	{
		Simulation.MenuKey(UGH_LOGIC_MENU_OTHER);
	}
}
