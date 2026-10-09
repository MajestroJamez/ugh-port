// The ghost of the best run as an automation test of the editor (Ugh.Ghost): its copters exactly where the replay's
// were in its last attempt's play, step by step with a new game's play, again in the next game; the game itself the same
// with it; which replay it flies (none when off, without a best, of another logic).
#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "UghGhost.h"
#include "UghReplayTestKit.h"
#include "UghReplays.h"
#include "UghSimulation.h"

using namespace UghReplayTestKit;

namespace
{
	/** Where the copters were at a step of an attempt's play. */
	struct FPlace
	{
		int32 Attempt = 0;   // from 1
		int32 PlayStep = 0;  // from 1
		int32 X[2] = {}, Y[2] = {};
	};

	/** A game of `Choice` the pilot flies, given up after `GiveUp` steps: its replay and where its copters were. */
	TSharedPtr<FUghReplay> Fly(const FUghGameChoice& Choice, uint32 Seed, int32 GiveUp, TArray<FPlace>& OutPlaces)
	{
		FUghSimulation Game;
		FString Problem;
		if (!Game.Load(DataFile(), Problem) || !Game.NewGame(Choice))
		{
			return nullptr;
		}
		FRandomPilot Pilot{ Seed };
		TSharedPtr<FUghReplay> Ended;
		int32 Attempts = 0;
		for (int32 Step = 0; Step < GiveUp + 2000 && !Game.IsOver(); ++Step)
		{
			Pilot.Fly(Game, Choice.Players);
			if (Step == GiveUp)
			{
				Game.MenuKey(UGH_LOGIC_MENU_ESCAPE);
			}
			Game.Advance(StepSeconds);
			for (const ugh_logic_event& Event : Game.GetEvents())
			{
				Attempts += Event.kind == UGH_LOGIC_EVENT_LEVEL_CAPTION;
			}
			const ugh_logic_view& View = Game.GetCurrent();
			if (Game.GetPlaySteps() > 0)
			{
				FPlace& Place = OutPlaces.AddDefaulted_GetRef();
				Place.Attempt = Attempts;
				Place.PlayStep = Game.GetPlaySteps();
				for (int32 Copter = 0; Copter < View.copter_count; ++Copter)
				{
					Place.X[Copter] = View.copters[Copter].x;
					Place.Y[Copter] = View.copters[Copter].y;
				}
			}
			Ended = Ended ? Ended : Game.TakeEndedLevel();
		}
		return Ended;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghGhostTest, "Ugh.Ghost",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghGhostTest::RunTest(const FString&)
{
	const FUghGameChoice Choices[] = { { 1, 1, 0 }, { 2, 0, 3 } };
	for (const FUghGameChoice& Choice : Choices)
	{
		const FString What = Choice.Players == 2 ? TEXT("team: ") : TEXT("one player: ");
		TArray<FPlace> Places;
		const TSharedPtr<FUghReplay> Replay = Fly(Choice, 7 + Choice.Players, 4000, Places);
		if (!TestTrue(What + TEXT("a replay of the level"), Replay.IsValid()))
		{
			return false;
		}
		const int32 Last = Replay->GetInfo().attempts;
		TMap<int32, const FPlace*> LastPlay;   // the replay's last attempt, by its play's step
		for (const FPlace& Place : Places)
		{
			if (Place.Attempt == Last)
			{
				LastPlay.Add(Place.PlayStep, &Place);
			}
		}
		// two new games of the level, a pilot hovering (no keys but those going on from the captions); the ghost along
		FUghGhost Ghost;
		FString Problem;
		if (!TestTrue(What + TEXT("the ghost's logic"), Ghost.Load(DataFile(), Problem)))
		{
			return false;
		}
		Ghost.SetReplay(Replay);
		for (int32 Game = 0; Game < 2; ++Game)
		{
			FUghSimulation Played, Alone;
			if (!Played.Load(DataFile(), Problem) || !Alone.Load(DataFile(), Problem) || !Played.NewGame(Choice) ||
				!Alone.NewGame(Choice))
			{
				return false;
			}
			int32 Shown = 0, Same = 0, Ready = 0, PlaySteps = 0;
			bool bGameAlike = true;
			for (int32 Step = 0; Step < 1500; ++Step)
			{
				if (Step % 40 == 0)
				{
					Played.MenuKey(UGH_LOGIC_MENU_OTHER);   // (on from the caption)
					Alone.MenuKey(UGH_LOGIC_MENU_OTHER);
				}
				Played.Advance(StepSeconds);
				Alone.Advance(StepSeconds);
				Ready += Played.GetPlaySteps() == 1 && Ghost.IsReady();   // (made ready before the play)
				Ghost.Follow(Played.GetPlaySteps());
				bGameAlike &= HashOf(Played.GetCurrent()) == HashOf(Alone.GetCurrent());
				PlaySteps = FMath::Max(PlaySteps, Played.GetPlaySteps());
				if (!Ghost.IsShown())
				{
					continue;
				}
				++Shown;
				const FPlace* Expected = LastPlay.FindRef(Played.GetPlaySteps());
				const ugh_logic_view& View = Ghost.GetCurrent();
				bool bThere = Expected && Ghost.GetSteps() == Played.GetPlaySteps() && View.copter_count == Choice.Players;
				for (int32 Copter = 0; bThere && Copter < Choice.Players; ++Copter)
				{
					bThere = View.copters[Copter].x == Expected->X[Copter] && View.copters[Copter].y == Expected->Y[Copter];
				}
				Same += bThere;
			}
			const FString Which = What + FString::Printf(TEXT("game %d: "), Game + 1);
			TestTrue(Which + TEXT("ready at the play's start"), Ready >= 1);
			TestTrue(FString::Printf(TEXT("%sshown through the replay's play (%d steps, the game's play %d)"), *Which, Shown,
				PlaySteps), Shown > 100 && Shown <= LastPlay.Num() * Ready);
			TestEqual(Which + TEXT("its copters where the replay's were, every step"), Same, Shown);
			TestTrue(Which + TEXT("the game the same with it"), bGameAlike);
		}
		Ghost.Follow(0);
		Ghost.SetReplay(nullptr);
		Ghost.Follow(10);
		TestFalse(What + TEXT("no replay: no ghost"), Ghost.IsShown());
	}

	// which replay: the best kept of the level and mode, of the same logic; none when off or without one
	char Problem[256] = "";
	ugh_logic* Logic = ugh_logic_create(TCHAR_TO_UTF8(*DataFile()), Problem, sizeof Problem);
	if (!TestNotNull(TEXT("the logic"), Logic))
	{
		return false;
	}
	const FString Folder = TestFolder();
	FUghReplays Store;
	Store.Open(Folder, Logic, true);
	FCraft Best;
	Best.DataHash = ugh_logic_data_hash(Logic);
	TestTrue(TEXT("no best: no ghost"), !FUghGhost::Of(Store, 1, 2, true));
	Store.OfferBest(Best.Replay());
	TestTrue(TEXT("the best flies"), FUghGhost::Of(Store, 1, 2, true) && FUghGhost::Of(Store, 1, 2, true)->Level() == 2);
	TestTrue(TEXT("off: none"), !FUghGhost::Of(Store, 1, 2, false));
	TestTrue(TEXT("another level, the team: none"), !FUghGhost::Of(Store, 1, 3, true) && !FUghGhost::Of(Store, 2, 2, true));
	FCraft Other = Best;
	Other.Level = 4;
	Other.DataHash ^= 1;
	Store.OfferBest(Other.Replay());
	TestTrue(TEXT("a best of other data: none"), Store.Best(1, 4) && !FUghGhost::Of(Store, 1, 4, true));
	IFileManager::Get().DeleteDirectory(*Folder, false, true);
	ugh_logic_destroy(Logic);
	return true;
}

#endif
