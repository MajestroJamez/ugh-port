#include "UghControls.h"

#include "Algo/BinarySearch.h"
#include "UghKeyBindings.h"
#include "ugh_logic.h"

namespace
{
	/** A gamepad's button and the logic key it flies. */
	struct FGamepadAction
	{
		FKey Key;
		int32 Action;   // UGH_LOGIC_KEY_...
	};

	const TArray<FGamepadAction>& GamepadActions()
	{
		static const TArray<FGamepadAction> Actions = {
			{ EKeys::Gamepad_LeftStick_Up, UGH_LOGIC_KEY_UP }, { EKeys::Gamepad_DPad_Up, UGH_LOGIC_KEY_UP },
			{ EKeys::Gamepad_LeftStick_Down, UGH_LOGIC_KEY_DOWN }, { EKeys::Gamepad_DPad_Down, UGH_LOGIC_KEY_DOWN },
			{ EKeys::Gamepad_LeftStick_Left, UGH_LOGIC_KEY_LEFT }, { EKeys::Gamepad_DPad_Left, UGH_LOGIC_KEY_LEFT },
			{ EKeys::Gamepad_LeftStick_Right, UGH_LOGIC_KEY_RIGHT }, { EKeys::Gamepad_DPad_Right, UGH_LOGIC_KEY_RIGHT },
			{ EKeys::Gamepad_FaceButton_Bottom, UGH_LOGIC_KEY_FIRE }, { EKeys::Gamepad_RightTrigger, UGH_LOGIC_KEY_FIRE },
		};
		return Actions;
	}
}

int32 FUghControls::GamepadAction(const FKey& Key)
{
	const FGamepadAction* Found =
		GamepadActions().FindByPredicate([&](const FGamepadAction& Action) { return Action.Key == Key; });
	return Found ? Found->Action : INDEX_NONE;
}

const FKey& FUghControls::BackKey()
{
	return EKeys::Gamepad_FaceButton_Right;
}

FKey FUghControls::MenuKeyOf(const FKey& Key)
{
	if (Key.IsMouseButton() || Key.IsTouch())
	{
		return FKey();
	}
	if (!Key.IsGamepadKey())
	{
		return Key;
	}
	static const FKey Arrows[] = { EKeys::Up, EKeys::Down, EKeys::Left, EKeys::Right };
	const int32 Action = GamepadAction(Key);
	if (Action >= UGH_LOGIC_KEY_UP && Action <= UGH_LOGIC_KEY_RIGHT)
	{
		return Arrows[Action - UGH_LOGIC_KEY_UP];
	}
	if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_Special_Right)
	{
		return EKeys::Enter;
	}
	if (Key == EKeys::Gamepad_FaceButton_Left)
	{
		return EKeys::BackSpace;
	}
	return Key == BackKey() || Key == EKeys::Gamepad_Special_Left ? BackKey() : FKey();
}

int32 FUghControls::PilotOf(FInputDeviceId Device)
{
	const int32 Id = Device.GetId();
	int32 Index = Algo::LowerBound(Gamepads, Id);
	if (Index == Gamepads.Num() || Gamepads[Index] != Id)
	{
		Gamepads.Insert(Id, Index);
	}
	return Index;
}

void FUghControls::Handle(IUghLogicInput& Logic, const FKey& Key, EInputEvent Event, FInputDeviceId Device)
{
	if (!Key.IsValid() || Key.IsMouseButton() || (Event != IE_Pressed && Event != IE_Released))
	{
		return;
	}
	const bool bPressed = Event == IE_Pressed;
	if (Key.IsGamepadKey())
	{
		const int32 Pilot = PilotOf(Device);
		const int32 Action = GamepadAction(Key);
		if (Action != INDEX_NONE && Pilot < FUghKeyBindings::Pilots)
		{
			Hold(Logic, Pilot, Action, FString::Printf(TEXT("%s@%d"), *Key.ToString(), Device.GetId()), bPressed);
		}
	}
	else if (const TOptional<FUghKeyBindings::FPlace> Place = Bindings.Find(Key))
	{
		Hold(Logic, Place->Pilot, Place->Action, Key.ToString(), bPressed);
	}
	if (bPressed && (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Left))
	{
		Logic.MenuKey(UGH_LOGIC_MENU_ESCAPE);
	}
	else if (bPressed && (Key == EKeys::P || Key == EKeys::Gamepad_Special_Right))
	{
		Logic.MenuKey(UGH_LOGIC_MENU_PAUSE);
	}
	else
	{
		Logic.MenuKey(UGH_LOGIC_MENU_OTHER);
	}
}

void FUghControls::Hold(IUghLogicInput& Logic, int32 Pilot, int32 Action, const FString& Source, bool bPressed)
{
	TSet<FString>& Holding = Held.FindOrAdd(Pilot * FUghKeyBindings::Actions + Action);
	const bool bWasHeld = !Holding.IsEmpty();
	if (bPressed)
	{
		Holding.Add(Source);
	}
	else
	{
		Holding.Remove(Source);
	}
	if (bWasHeld != !Holding.IsEmpty())
	{
		Logic.Key(Pilot, Action, bPressed);
	}
}
