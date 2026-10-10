// Each pilot's passenger in the status of the play, as an automation test of the editor (Ugh.Ui.Cargo).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghHud.h"
#include "UghKnockPilot.h"
#include "UghPadSigns.h"
#include "UghUiState.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	/** The logic plays this long at most (steps). */
	constexpr int32 MostSteps = 12000;

	/** What the status showed of the flown pilot's passenger through a level. */
	struct FCargoSeen
	{
		bool bPlayed = false, bBoarded = false, bLeft = false, bDelivered = false, bFell = false, bLost = false;
		int32 Destination = 0, FirstFare = -1, LastFare = -1;
	};

	/**
	 * Level 1 of the mode of `Players` with copter `Player` flown by FUghFarePilot (the other one stays): every step the
	 * status (AUghHud::ShowCargo) shows each copter's passenger as the logic has it - the number of the pad it wants, its
	 * marks, its fare - and nothing without one.
	 */
	FCargoSeen Fly(FAutomationTestBase& Test, ugh_logic* Logic, int32 Players, int32 Player)
	{
		ugh_logic_settings Settings;
		ugh_logic_default_settings(&Settings);
		Settings.players = Players;
		ugh_logic_new_game(Logic, &Settings);
		FUghFarePilot Pilot(Player);
		FUghUiState Shown;
		ugh_logic_view Previous{}, Current{};
		bool Held[3] = { false, false, false };   // up, left, right
		FCargoSeen Seen;
		bool bMismatch = false, bOtherUp = false;
		int32 OtherY = -1;
		for (int32 Step = 0; Step < MostSteps && !Pilot.HasDelivered() && !Pilot.IsLost(); ++Step)
		{
			Previous = Current;
			ugh_logic_step(Logic);
			ugh_logic_take_events(Logic, [](void*, const ugh_logic_event*) {}, nullptr);
			ugh_logic_get_view(Logic, &Current);
			if (Current.phase == UGH_LOGIC_PHASE_CAPTION && Step % 20 == 0)
			{
				ugh_logic_menu_key(Logic, UGH_LOGIC_MENU_OTHER);
			}
			const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Previous.phase == UGH_LOGIC_PHASE_PLAY;
			if (Seen.bPlayed && !bPlay)
			{
				Seen.bFell = true;   // (the attempt ended: a crash)
				break;
			}
			Seen.bPlayed |= bPlay;
			if (!bPlay)
			{
				continue;
			}
			AUghHud::ShowCargo(Shown, Current);
			for (int32 Each = 0; Each < Current.copter_count; ++Each)
			{
				const ugh_logic_copter& Copter = Current.copters[Each];
				const FUghUiState::FCargo& Cargo = Shown.Cargo[Each];
				const bool bRight = Copter.destination > 0
					? Cargo.Destination == Copter.destination && Cargo.Marks == UghPadSigns::Marks(Copter.destination) &&
						Cargo.Fare == FMath::Max(Copter.fare, 0)
					: Cargo.Destination == 0;
				if (!bRight && !bMismatch)
				{
					Test.AddError(FString::Printf(TEXT("%dp, copter %d, step %d: the status shows pad %d (marks %d, fare %d), "
						"the logic has %d (fare %d)"), Players, Each, Step, Cargo.Destination, Cargo.Marks, Cargo.Fare,
						Copter.destination, Copter.fare));
					bMismatch = true;
				}
			}
			const FUghUiState::FCargo& Flown = Shown.Cargo[Player];
			if (Flown.Destination > 0)
			{
				Seen.bBoarded = true;
				Seen.Destination = Flown.Destination;
				Seen.FirstFare = Seen.FirstFare < 0 ? Flown.Fare : Seen.FirstFare;
				Seen.LastFare = Flown.Fare;
			}
			Seen.bLeft |= Seen.bBoarded && Flown.Destination == 0;
			const FUghPilotKeys Keys = Pilot.Fly(Logic, Previous, Current);
			const bool Wanted[3] = { Keys.bUp, Keys.bLeft, Keys.bRight };
			const int32 LogicKeys[3] = { UGH_LOGIC_KEY_UP, UGH_LOGIC_KEY_LEFT, UGH_LOGIC_KEY_RIGHT };
			for (int32 Key = 0; Key < 3; ++Key)
			{
				if (Held[Key] != Wanted[Key])
				{
					ugh_logic_key(Logic, Player, LogicKeys[Key], Wanted[Key] ? 1 : 0);
					Held[Key] = Wanted[Key];
				}
			}
			if (Current.copter_count > 1)
			{
				// the other copter hovers where it began (it would sink into the sea: an attempt of the team lost)
				const int32 Other = 1 - Player;
				OtherY = OtherY < 0 ? Current.copters[Other].y : OtherY;
				const bool bUp = Current.copters[Other].y + (Current.copters[Other].y - Previous.copters[Other].y) * 12 > OtherY;
				if (bUp != bOtherUp)
				{
					ugh_logic_key(Logic, Other, UGH_LOGIC_KEY_UP, bUp ? 1 : 0);
					bOtherUp = bUp;
				}
			}
		}
		Seen.bDelivered = Pilot.HasDelivered();
		Seen.bLost = Pilot.IsLost();
		return Seen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghUiCargoTest, "Ugh.Ui.Cargo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The status of the play shows each pilot's passenger as the original's status bar (step 32h): in level 1 of one player
 * and of the team (each copter flown in turn) a passenger got in - the status shows the pad its copter's destination
 * names (its number and marks) and its fare, falling while it rides -, flown to that pad it got out and the status
 * shows nothing again; the other pilot's part nothing all along. The stone on the sling (destination -1) and an empty
 * cabin show nothing, nor a passenger out of the play.
 */
bool FUghUiCargoTest::RunTest(const FString& Parameters)
{
	char Problem[256] = {};
	ugh_logic* Logic = ugh_logic_create(TCHAR_TO_UTF8(*(Assets() / TEXT("logic/ugh-data.ugd"))), Problem, sizeof Problem);
	if (!TestNotNull(FString::Printf(TEXT("the game data: %hs"), Problem), Logic))
	{
		return false;
	}
	ON_SCOPE_EXIT { ugh_logic_destroy(Logic); };
	const TPair<int32, int32> Flights[] = { { 1, 0 }, { 2, 0 }, { 2, 1 } };
	for (const TPair<int32, int32>& Flight : Flights)
	{
		const FCargoSeen Seen = Fly(*this, Logic, Flight.Key, Flight.Value);
		const FString Who = FString::Printf(TEXT("%dp, pilot %d"), Flight.Key, Flight.Value + 1);
		TestTrue(Who + TEXT(": the level played"), Seen.bPlayed);
		TestFalse(Who + TEXT(": the pilot found a passenger, its pad and the ways"), Seen.bLost);
		TestFalse(Who + TEXT(": no crash"), Seen.bFell);
		TestTrue(Who + TEXT(": a passenger got in, the status showed its pad"), Seen.bBoarded && Seen.Destination > 0);
		TestTrue(FString::Printf(TEXT("%s: its fare fell while it rode (%d to %d)"), *Who, Seen.FirstFare, Seen.LastFare),
			Seen.LastFare >= 0 && Seen.LastFare < Seen.FirstFare);
		TestTrue(Who + TEXT(": it got out at its pad, the status shows nothing"), Seen.bDelivered && Seen.bLeft);
	}

	// the stone on the sling, an empty cabin, a passenger out of the play: nothing
	ugh_logic_view View{};
	View.phase = UGH_LOGIC_PHASE_PLAY;
	View.copter_count = 2;
	View.copters[0].destination = -1;
	View.copters[0].cargo_look = 4;
	View.copters[0].fare = 0;
	View.copters[1].destination = 0;
	FUghUiState Shown;
	Shown.Cargo[0].Destination = Shown.Cargo[1].Destination = 3;
	AUghHud::ShowCargo(Shown, View);
	TestTrue(TEXT("the stone on the sling and an empty cabin show nothing"),
		Shown.Cargo[0].Destination == 0 && Shown.Cargo[1].Destination == 0);
	View.copters[1].destination = 2;
	View.copters[1].fare = 40;
	View.phase = UGH_LOGIC_PHASE_CAPTION;
	AUghHud::ShowCargo(Shown, View);
	TestEqual(TEXT("nothing out of the play"), Shown.Cargo[1].Destination, 0);
	View.phase = UGH_LOGIC_PHASE_PLAY;
	View.copters[1].destination = 7;
	AUghHud::ShowCargo(Shown, View);
	TestTrue(TEXT("a pad of no board in the bubbles: its number, a blank board"),
		Shown.Cargo[1].Destination == 7 && Shown.Cargo[1].Marks == 0 && Shown.Cargo[1].Fare == 40);
	return true;
}

#endif
