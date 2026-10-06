// The keys and the gamepads to the inputs of the logic.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Misc/CoreMiscDefines.h"

struct FUghKeyBindings;

/** Where the inputs of the logic go (FUghSimulation; a test records them). */
class IUghLogicInput
{
public:
	virtual ~IUghLogicInput() = default;
	/** A pilot's key (UGH_LOGIC_KEY_...), player 0 or 1. */
	virtual void Key(int32 Player, int32 LogicKey, bool bPressed) = 0;
	/** A key the game loop sees (UGH_LOGIC_MENU_...). */
	virtual void MenuKey(int32 LogicMenuKey) = 0;
};

/**
 * The controls of the remake (Adapter): a key event of the engine - a key of the keyboard or a button of a gamepad -
 * to the inputs of the logic, as the original's keyboard. The keyboard: the pilots' keys (FUghKeyBindings, set in the
 * menu), Esc gives the game up, P pauses; a gamepad flies its pilot (the first one pilot 1, the second pilot 2, in the
 * order of the engine's device ids): the left stick or the d-pad fly, A or the right trigger fires, Start pauses, Back
 * gives the game up. Every other press and every release is a key the game loop sees (a caption waits for one), as in
 * the original. A logic key held by two keys (Space and Right Ctrl, the stick and the d-pad) is released with the
 * last of them. In the menu a gamepad's buttons are the menu's keys (MenuKeyOf).
 */
class FUghControls
{
public:
	explicit FUghControls(const FUghKeyBindings& InBindings) : Bindings(InBindings) {}

	/** A key event in a game; key repeats are not key events of the original. */
	void Handle(IUghLogicInput& Logic, const FKey& Key, EInputEvent Event, FInputDeviceId Device);
	/** A new game: nothing is held. */
	void Reset() { Held.Reset(); }

	/** The pilot a gamepad flies (0, 1): its place among the gamepads seen, by device id. */
	int32 PilotOf(FInputDeviceId Device);

	/**
	 * The key of the keyboard a gamepad's button is in the menu: the stick and the d-pad the arrows, A and Start Enter,
	 * X Backspace; B and Back are the menu's Back (BackKey); the keyboard's keys as they are; none for the others
	 * (the mouse's too).
	 */
	static FKey MenuKeyOf(const FKey& Key);
	/** The gamepad's B in the menu: back (on the title screen nothing). */
	static const FKey& BackKey();
	/** The logic key (UGH_LOGIC_KEY_...) a gamepad's button flies, INDEX_NONE for none. */
	static int32 GamepadAction(const FKey& Key);

private:
	/** The logic's key `Action` of `Pilot` held by one more or one less key; the logic hears the first and the last. */
	void Hold(IUghLogicInput& Logic, int32 Pilot, int32 Action, const FString& Source, bool bPressed);

	const FUghKeyBindings& Bindings;
	/** What holds each pilot's logic key down: the names of the keys (and the gamepad's device). */
	TMap<int32, TSet<FString>> Held;
	TArray<int32> Gamepads;   // the device ids seen, sorted
};
