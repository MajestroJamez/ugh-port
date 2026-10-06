// Which keys the pilots fly with.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class FJsonObject;

/**
 * The keys of both pilots: for each of the logic's keys (UGH_LOGIC_KEY_UP, DOWN, LEFT, RIGHT, FIRE) two keys of the
 * keyboard, either may be empty. The defaults are the remake's: pilot 1 the arrows and Right Ctrl or Space, pilot 2
 * W A S D and Left Ctrl (the original's Z for down is Y on a Czech keyboard, so S instead). A key flies one thing at a
 * time: binding it where another one is swaps the two. The keys of the game itself (Esc, P, F1, U, G, Page Up, Page
 * Down), the mouse's and the gamepads' cannot be bound.
 */
struct FUghKeyBindings
{
	static constexpr int32 Pilots = 2, Actions = 5, Slots = 2;

	/** Where a key is bound: a pilot (0, 1), a logic key (UGH_LOGIC_KEY_...), a slot (0, 1). */
	struct FPlace
	{
		int32 Pilot = 0, Action = 0, Slot = 0;
		bool operator==(const FPlace& Other) const = default;
	};

	/** What binding a key did. */
	enum class EBound : uint8 { Bound, Swapped, Reserved };

	FKey Keys[Pilots][Actions][Slots];

	static FUghKeyBindings Defaults();

	const FKey& Get(const FPlace& Place) const { return Keys[Place.Pilot][Place.Action][Place.Slot]; }
	/** The first key of pilot `Pilot` for `Action` (the one a help shows), none when both are empty. */
	FKey KeyOf(int32 Pilot, int32 Action) const;
	/** Where `Key` is bound, if anywhere. */
	TOptional<FPlace> Find(const FKey& Key) const;

	/**
	 * Binds `Key` at `Place`: where it was bound before gets the key `Place` had (`OutOther`, Swapped); a reserved key
	 * is refused (Reserved).
	 */
	EBound Bind(const FPlace& Place, const FKey& Key, FPlace* OutOther = nullptr);
	void Clear(const FPlace& Place) { Keys[Place.Pilot][Place.Action][Place.Slot] = FKey(); }

	/** The game's own keys, the mouse's and the gamepads': never a pilot's. */
	static bool IsReserved(const FKey& Key);
	/** A key as the screen writes it: short (arrows as arrows), "-" for none. */
	static FString NameOf(const FKey& Key);
	/** The name of a logic key (Up, Down, Left, Right, Fire). */
	static const TCHAR* ActionName(int32 Action);

	/** As JSON: {"pilot1": {"up": ["Up", ""], ...}, "pilot2": ...} (the engine's key names). */
	TSharedRef<FJsonObject> ToJson() const;
	/** From JSON; what is missing or not a key keeps the default. */
	static FUghKeyBindings FromJson(const FJsonObject& Json);

	bool operator==(const FUghKeyBindings& Other) const;
};
