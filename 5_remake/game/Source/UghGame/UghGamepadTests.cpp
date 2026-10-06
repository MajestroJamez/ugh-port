// The gamepads as automation tests of the editor (Ugh.Gamepad): their buttons give the logic what the keys give, the
// first pad pilot 1's, the second pilot 2's; in the menu they are its keys; a copter flies up by the stick as by the
// arrow.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghControls.h"
#include "UghKeyBindings.h"
#include "UghLogicRecorder.h"
#include "UghSimulation.h"

namespace
{
	/** A key event as the player controller passes it to the game mode (its device apart). */
	struct FPress
	{
		FKey Key;
		EInputEvent Event;
	};

	/** Each event of `Presses` through FUghControls from `Device`, as the game mode passes them in a game. */
	FString Play(FUghControls& Controls, const TArray<FPress>& Presses, FInputDeviceId Device)
	{
		FUghLogicRecorder Logic;
		for (const FPress& Press : Presses)
		{
			Controls.Handle(Logic, Press.Key, Press.Event, Device);
		}
		return Logic.Take();
	}

	/**
	 * A new game of one player: `Fire` (from `Device`) tapped through the caption, then `Up` held for 200 steps of the
	 * play (none: nothing); where the copter is then (its y in 1/32 px).
	 */
	int32 FlyUp(FUghSimulation& Simulation, const FKey& Fire, const FKey& Up, FInputDeviceId Device)
	{
		const FUghKeyBindings Keys = FUghKeyBindings::Defaults();
		FUghControls Controls(Keys);
		Simulation.NewGame({ 1, 1, 0 });
		const double Frame = 1.0001 / FUghSimulation::TickRate;
		int32 Held = 0;
		for (int32 Step = 0; Step < 3000 && Held < 200; ++Step)
		{
			const int32 Phase = Simulation.GetCurrent().phase;
			if (Phase == UGH_LOGIC_PHASE_CAPTION && Step % 20 == 0)
			{
				Controls.Handle(Simulation, Fire, Step % 40 == 0 ? IE_Pressed : IE_Released, Device);
			}
			if (Phase == UGH_LOGIC_PHASE_PLAY && Held++ == 0 && Up.IsValid())
			{
				Controls.Handle(Simulation, Up, IE_Pressed, Device);
			}
			Simulation.Advance(Frame);
		}
		return Simulation.GetCurrent().copters[0].y;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghGamepadTest, "Ugh.Gamepad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghGamepadTest::RunTest(const FString& Parameters)
{
	const FUghKeyBindings Keys = FUghKeyBindings::Defaults();
	const FInputDeviceId Keyboard = FInputDeviceId::CreateFromInternalId(0);
	// any device ids: the lower one is pilot 1's, whichever is used first
	const FInputDeviceId First = FInputDeviceId::CreateFromInternalId(3), Second = FInputDeviceId::CreateFromInternalId(7);
	FUghControls Pads(Keys);
	TestEqual(TEXT("the second pad seen first is pilot 2's"), Pads.PilotOf(Second), 0);
	TestEqual(TEXT("then the first pad is pilot 1's"), Pads.PilotOf(First), 0);
	TestEqual(TEXT("and the second pilot 2's"), Pads.PilotOf(Second), 1);

	// both pilots, the keyboard's keys and the pads' buttons
	const TArray<FPress> ByKeys1 = { { EKeys::Up, IE_Pressed }, { EKeys::Right, IE_Pressed }, { EKeys::Up, IE_Released },
		{ EKeys::RightControl, IE_Pressed }, { EKeys::RightControl, IE_Released }, { EKeys::Right, IE_Released },
		{ EKeys::P, IE_Pressed }, { EKeys::Escape, IE_Pressed } };
	const TArray<FPress> ByPad1 = { { EKeys::Gamepad_LeftStick_Up, IE_Pressed }, { EKeys::Gamepad_DPad_Right, IE_Pressed },
		{ EKeys::Gamepad_LeftStick_Up, IE_Released }, { EKeys::Gamepad_FaceButton_Bottom, IE_Pressed },
		{ EKeys::Gamepad_FaceButton_Bottom, IE_Released }, { EKeys::Gamepad_DPad_Right, IE_Released },
		{ EKeys::Gamepad_Special_Right, IE_Pressed }, { EKeys::Gamepad_Special_Left, IE_Pressed } };
	const TArray<FPress> ByKeys2 = { { EKeys::W, IE_Pressed }, { EKeys::A, IE_Pressed }, { EKeys::LeftControl, IE_Pressed },
		{ EKeys::W, IE_Released }, { EKeys::A, IE_Released }, { EKeys::LeftControl, IE_Released } };
	const TArray<FPress> ByPad2 = { { EKeys::Gamepad_DPad_Up, IE_Pressed }, { EKeys::Gamepad_LeftStick_Left, IE_Pressed },
		{ EKeys::Gamepad_RightTrigger, IE_Pressed }, { EKeys::Gamepad_DPad_Up, IE_Released },
		{ EKeys::Gamepad_LeftStick_Left, IE_Released }, { EKeys::Gamepad_RightTrigger, IE_Released } };
	FUghControls ByKeyboard(Keys);
	const FString Pilot1 = Play(ByKeyboard, ByKeys1, Keyboard);
	TestEqual(TEXT("pilot 1: the first pad gives the logic what the keys give"), Play(Pads, ByPad1, First), Pilot1);
	TestTrue(TEXT("(all of it)"), Pilot1.StartsWith(TEXT("key 0 up on, menu other, key 0 right on")) &&
		Pilot1.EndsWith(TEXT("menu pause, menu escape")));
	const FString Pilot2 = Play(ByKeyboard, ByKeys2, Keyboard);
	TestEqual(TEXT("pilot 2: the second pad as W A S D and Left Ctrl"), Play(Pads, ByPad2, Second), Pilot2);
	TestTrue(TEXT("(pilot 2's)"), Pilot2.StartsWith(TEXT("key 1 up on")));

	// the stick and the d-pad together: up until both let go (as Space and Right Ctrl)
	const FString Both = Play(Pads, { { EKeys::Gamepad_LeftStick_Up, IE_Pressed }, { EKeys::Gamepad_DPad_Up, IE_Pressed },
		{ EKeys::Gamepad_LeftStick_Up, IE_Released }, { EKeys::Gamepad_DPad_Up, IE_Released } }, First);
	TestEqual(TEXT("held by the stick and the d-pad"), Both,
		FString(TEXT("key 0 up on, menu other, menu other, menu other, key 0 up off, menu other")));
	TestEqual(TEXT("as by Space and Right Ctrl"), Play(ByKeyboard, { { EKeys::SpaceBar, IE_Pressed },
		{ EKeys::RightControl, IE_Pressed }, { EKeys::SpaceBar, IE_Released }, { EKeys::RightControl, IE_Released } }, Keyboard),
		FString(TEXT("key 0 fire on, menu other, menu other, menu other, key 0 fire off, menu other")));
	TestEqual(TEXT("a third pad flies nobody"), Play(Pads, { { EKeys::Gamepad_DPad_Up, IE_Pressed } },
		FInputDeviceId::CreateFromInternalId(9)), FString(TEXT("menu other")));

	// the menu: the pad's buttons are its keys
	TestEqual(TEXT("d-pad down"), FUghControls::MenuKeyOf(EKeys::Gamepad_DPad_Down), EKeys::Down);
	TestEqual(TEXT("the stick left"), FUghControls::MenuKeyOf(EKeys::Gamepad_LeftStick_Left), EKeys::Left);
	TestEqual(TEXT("A selects"), FUghControls::MenuKeyOf(EKeys::Gamepad_FaceButton_Bottom), EKeys::Enter);
	TestEqual(TEXT("B goes back"), FUghControls::MenuKeyOf(EKeys::Gamepad_FaceButton_Right), FUghControls::BackKey());
	TestEqual(TEXT("X deletes"), FUghControls::MenuKeyOf(EKeys::Gamepad_FaceButton_Left), EKeys::BackSpace);
	TestEqual(TEXT("a key as it is"), FUghControls::MenuKeyOf(EKeys::K), EKeys::K);
	TestFalse(TEXT("the mouse nothing"), FUghControls::MenuKeyOf(EKeys::LeftMouseButton).IsValid());

	// a game: the copter flies up by the pad as by the keyboard
	FUghSimulation Simulation;
	FString Error;
	const FString Data = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets/logic/ugh-data.ugd"));
	if (!TestTrue(TEXT("the data: ") + Error, Simulation.Load(Data, Error)))
	{
		return false;
	}
	const int32 Resting = FlyUp(Simulation, EKeys::RightControl, FKey(), Keyboard);
	const int32 ByArrow = FlyUp(Simulation, EKeys::RightControl, EKeys::Up, Keyboard);
	const int32 ByStick = FlyUp(Simulation, EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_LeftStick_Up, First);
	TestTrue(FString::Printf(TEXT("the arrow flies up (%d above %d)"), ByArrow, Resting), ByArrow < Resting);
	TestEqual(TEXT("the stick as high"), ByStick, ByArrow);
	return true;
}

#endif
