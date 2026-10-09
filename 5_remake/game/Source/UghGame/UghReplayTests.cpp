// The replays of the levels as automation tests of the editor (Ugh.Record.*): the store (the best of each level and
// mode, saved, imported, refused, deleted), a replay written by hand as docs/replay-format.md says, a game's replay
// watched through the game's own simulation, the menu's screen and the level selection's key.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "UghJson.h"
#include "UghMenu.h"
#include "UghMenuView.h"
#include "UghPasswords.h"
#include "UghProfile.h"
#include "UghReplays.h"
#include "UghSimulation.h"
#include "UghStage.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }
	const FString DataFile() { return Assets() / TEXT("logic/ugh-data.ugd"); }

	/** A folder of its own for a test's replays (deleted at its end). */
	FString TestFolder()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / TEXT("Replays-") +
			FGuid::NewGuid().ToString());
	}

	/** The CRC-32 of zip (as the format's checksum). */
	uint32 Crc32(const TArray<uint8>& Bytes)
	{
		uint32 Crc = 0xFFFFFFFFu;
		for (const uint8 Byte : Bytes)
		{
			Crc ^= Byte;
			for (int32 Bit = 0; Bit < 8; ++Bit)
			{
				Crc = Crc & 1 ? 0xEDB88320u ^ (Crc >> 1) : Crc >> 1;
			}
		}
		return ~Crc;
	}

	/** A replay written by hand as docs/replay-format.md says. */
	struct FCraft
	{
		int32 Players = 1, Difficulty = 1, Level = 2;
		uint32 Points = 1000;
		int32 Steps = 3000, PlaySteps = 2000;
		bool bDone = true;
		uint32 DataHash = 0;
		uint64 Date = 1791000000;
		FString Name = TEXT("Jan");

		TArray<uint8> Bytes() const
		{
			TArray<uint8> B;
			auto Number = [&B](uint64 Value)
			{
				for (; Value >= 0x80; Value >>= 7)
				{
					B.Add(uint8(Value | 0x80));
				}
				B.Add(uint8(Value));
			};
			auto Word = [&B](uint32 Value, int32 Bytes) { for (int32 I = 0; I < Bytes; ++I) B.Add(uint8(Value >> (8 * I))); };
			auto Text = [&B, &Number](const FString& Value)
			{
				const FTCHARToUTF8 Utf8(*Value);
				Number(Utf8.Length());
				B.Append(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
			};
			B.Append({ 'U', 'G', 'H', 'R', 1 });
			Number(UGH_LOGIC_VERSION);
			Word(DataHash, 4);
			B.Add(uint8(Players));
			B.Add(uint8(Difficulty));
			Number(Level);
			Number(3);        // lives
			Number(5000);     // the points before
			Number(1);        // multiplier
			for (const uint16 Random : { 0x0003, 0x8134, 0x48bc, 0x2347 })
			{
				Word(Random, 2);
			}
			B.Add(180);       // the rain's row
			for (int32 Player = 0; Player < Players; ++Player)
			{
				Number(0);    // effort 0 (zigzag)
			}
			B.Add(2);         // the last menu key: Other
			Number(Steps);
			Number(PlaySteps);
			Number(1);        // attempts
			Number(Points);
			B.Add(bDone ? 1 : 0);
			Number(Date);
			Text(TEXT("PASSWORD"));
			B.Add(1);
			Text(Name);
			Number(2);        // inputs: Up pressed with Other after it after step 10, released after step 30
			Number((10ull << 6) | (0 * 10 + 0 * 2 + 1 + 20));
			Number((20ull << 6) | (0 * 10 + 0 * 2 + 0));
			Word(Crc32(B), 4);
			return B;
		}
		TSharedPtr<FUghReplay> Replay() const
		{
			FString Error;
			return FUghReplay::Read(Bytes(), Error);
		}
	};

	/** A pilot holding random keys a random while (as the frontend gives them: each key event to the game loop too). */
	struct FRandomPilot
	{
		uint32 State;
		bool Held[2][5] = {};
		void Fly(IUghLogicInput& Logic, int32 Players)
		{
			for (int32 Player = 0; Player < Players; ++Player)
			{
				for (int32 Key = UGH_LOGIC_KEY_UP; Key <= UGH_LOGIC_KEY_FIRE; ++Key)
				{
					State = State * 1664525u + 1013904223u;
					if ((State >> 8) % (Key == UGH_LOGIC_KEY_UP ? 12 : 30) == 0)
					{
						Held[Player][Key] = !Held[Player][Key];
						Logic.Key(Player, Key, Held[Player][Key]);
						Logic.MenuKey(UGH_LOGIC_MENU_OTHER);
					}
				}
			}
		}
	};

	uint32 HashOf(const ugh_logic_view& View) { return FCrc::MemCrc32(&View, sizeof View); }

	/** Real time of one step of the logic (and a little more: one step a call, never two). */
	constexpr double StepSeconds = 1.0 / FUghSimulation::TickRate + 1e-9;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRecordStoreTest, "Ugh.Record.Store",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRecordStoreTest::RunTest(const FString&)
{
	char Problem[256] = "";
	ugh_logic* Logic = ugh_logic_create(TCHAR_TO_UTF8(*DataFile()), Problem, sizeof Problem);
	if (!TestNotNull(TEXT("the logic"), Logic))
	{
		return false;
	}
	const uint32 Hash = ugh_logic_data_hash(Logic);
	const FString Folder = TestFolder();
	FUghReplays Store;
	Store.Open(Folder, Logic, true);
	TestEqual(TEXT("no replays at first"), Store.GetEntries().Num(), 0);

	// a replay written by hand as the format says: read with all it says
	FCraft Craft;
	Craft.DataHash = Hash;
	const TSharedPtr<FUghReplay> Hand = Craft.Replay();
	if (!TestTrue(TEXT("a replay written as docs/replay-format.md says is read"), Hand.IsValid()))
	{
		ugh_logic_destroy(Logic);
		return false;
	}
	const ugh_replay_info& Info = Hand->GetInfo();
	TestTrue(TEXT("its header as written"), Info.start.players == 1 && Info.start.difficulty == 1 && Info.start.level == 2 &&
		Info.start.lives == 3 && Info.start.score == 5000 && Info.start.random[3] == 0x2347 && Info.start.rain_floor_row == 180 &&
		Info.steps == 3000 && Info.play_steps == 2000 && Info.points == 1000 && Info.done == 1 && Info.date == 1791000000 &&
		FString(Info.password) == TEXT("PASSWORD") && Hand->Name() == TEXT("Jan") && Info.input_count == 3);
	TestEqual(TEXT("its bytes as written"), Hand->Bytes(), Craft.Bytes());
	TestEqual(TEXT("the same logic"), ugh_replay_compare_logic(Hand->GetHandle(), Logic), int32(UGH_REPLAY_SAME_LOGIC));
	TestTrue(TEXT("its text"), Hand->Text().StartsWith(TEXT("UGHR1:")));

	// the best of a level and mode: only done, more points, at the same points less time
	FCraft NotDone = Craft;
	NotDone.bDone = false;
	NotDone.Points = 9000;
	TestFalse(TEXT("a level not done is no best"), Store.OfferBest(NotDone.Replay()));
	TestTrue(TEXT("the first done is the best"), Store.OfferBest(Hand));
	TestTrue(TEXT("written as best-1p-03.ughr"), IFileManager::Get().FileExists(*(Folder / TEXT("best-1p-03.ughr"))));
	FCraft Fewer = Craft;
	Fewer.Points = 999;
	Fewer.PlaySteps = 100;
	TestFalse(TEXT("fewer points are not better, however quick"), Store.OfferBest(Fewer.Replay()));
	FCraft Quicker = Craft;
	Quicker.PlaySteps = 1999;
	TestTrue(TEXT("the same points quicker are"), Store.OfferBest(Quicker.Replay()));
	TestFalse(TEXT("the same again is not"), Store.OfferBest(Quicker.Replay()));
	FCraft More = Craft;
	More.Points = 1001;
	More.PlaySteps = 2500;
	TestTrue(TEXT("more points are, however slow"), Store.OfferBest(More.Replay()));
	FCraft Team = Craft;
	Team.Players = 2;
	TestTrue(TEXT("the team's level its own"), Store.OfferBest(Team.Replay()));
	TestTrue(TEXT("the best of each"), Store.Best(1, 2) && Store.Best(1, 2)->Replay->GetInfo().points == 1001 &&
		Store.Best(2, 2) && !Store.Best(1, 3));
	TestEqual(TEXT("one file a best"), Store.GetEntries().Num(), 2);

	// saved, imported (once), refused
	FString Error;
	const FString Saved = Store.Save(Hand, FUghReplays::EKind::Saved, Error);
	TestTrue(TEXT("saved as <mode>-<NN>-<date>"), Saved.StartsWith(TEXT("1p-03-")) && Saved.EndsWith(TEXT(".ughr")));
	FString Message;
	FCraft Friend = Craft;
	Friend.Name = TEXT("Petr");
	Friend.DataHash = Hash ^ 1;   // made with other data
	TestTrue(TEXT("a friend's text imported"), Store.Import(TEXT("  ") + Friend.Replay()->Text() + TEXT("\r\n"), Message).IsValid() &&
		Message.StartsWith(TEXT("Imported")) && Message.Contains(TEXT("another version")));
	TestTrue(TEXT("once"), Store.Import(Friend.Replay()->Text(), Message).IsValid() && Message.StartsWith(TEXT("Already")));
	FString Damaged = Friend.Replay()->Text();
	Damaged[20] = Damaged[20] == TEXT('A') ? TEXT('B') : TEXT('A');
	TestFalse(TEXT("a damaged one refused"), Store.Import(Damaged, Message).IsValid());
	TestTrue(TEXT("saying so: ") + Message, Message.Contains(TEXT("damaged")));
	TestFalse(TEXT("a foreign text refused"), Store.Import(TEXT("hello, a Word document"), Message).IsValid());
	TestTrue(TEXT("saying so: ") + Message, Message.Contains(TEXT("not a UGH! replay")));
	TestFalse(TEXT("a newer one refused"), Store.Import(TEXT("UGHR2:") + Friend.Replay()->Text().Mid(6), Message).IsValid());
	TestTrue(TEXT("saying so: ") + Message, Message.Contains(TEXT("newer")));
	TestFalse(TEXT("an empty clipboard"), Store.Import(TEXT(""), Message).IsValid());
	const FUghReplays::FEntry* Imported = Store.GetEntries().FindByPredicate(
		[](const FUghReplays::FEntry& Entry) { return Entry.Kind == FUghReplays::EKind::Imported; });
	TestTrue(TEXT("the imported one made with other data"), Imported && Imported->OtherLogic == UGH_REPLAY_OTHER_DATA);
	TestEqual(TEXT("the best, the saved, the imported"), Store.GetEntries().Num(), 4);

	// a file put into the folder: a replay (its text), a broken one
	FFileHelper::SaveStringToFile(Team.Replay()->Text(), *(Folder / TEXT("from a friend.ughr")));
	FFileHelper::SaveStringToFile(TEXT("not a replay at all"), *(Folder / TEXT("junk.ughr")));
	FUghReplays Again;
	Again.Open(Folder, Logic, true);
	TestEqual(TEXT("read back: all of them"), Again.GetEntries().Num(), 6);
	TestTrue(TEXT("the bests again"), Again.Best(1, 2) && Again.Best(1, 2)->Replay->GetInfo().points == 1001 && Again.Best(2, 2));
	const FUghReplays::FEntry& Last = Again.GetEntries().Last();
	TestTrue(TEXT("the broken one last, with why"), !Last.Replay && Last.Problem.Contains(TEXT("not a UGH! replay")) &&
		Last.File.EndsWith(TEXT("junk.ughr")));
	TestTrue(TEXT("in order: by mode and level, the best first"), Again.GetEntries()[0].Kind == FUghReplays::EKind::Best &&
		Again.GetEntries()[0].Replay->Players() == 1);
	TestTrue(TEXT("deleted"), Again.Delete(Again.GetEntries().Num() - 1, Error) &&
		!IFileManager::Get().FileExists(*(Folder / TEXT("junk.ughr"))));

	// the autopilot's store only reads
	FUghReplays Reading;
	Reading.Open(Folder, Logic, false);
	TestFalse(TEXT("the autopilot writes no best"), Reading.OfferBest(More.Replay()) || Reading.OfferBest(FCraft{ 1, 1, 9, 50000 }.Replay()));
	TestTrue(TEXT("nor saves"), Reading.Save(Hand, FUghReplays::EKind::Saved, Error).IsEmpty());

	IFileManager::Get().DeleteDirectory(*Folder, false, true);
	ugh_logic_destroy(Logic);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRecordPlayTest, "Ugh.Record.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRecordPlayTest::RunTest(const FString&)
{
	// a game of each mode, the pilot flying at random (crashing), given up: its level's replay watched again through the
	// game's simulation gives the same view step by step
	const FUghGameChoice Games[] = { { 1, 1, 0 }, { 2, 2, 4 }, { 1, 0, 24 } };
	for (const FUghGameChoice& Choice : Games)
	{
		const FString What = FString::Printf(TEXT("%s, level %d: "), Choice.Players == 2 ? TEXT("team") : TEXT("one player"),
			Choice.FirstLevel + 1);
		FUghSimulation Game, Watcher;
		FString Problem;
		if (!TestTrue(What + TEXT("the data"), Game.Load(DataFile(), Problem) && Watcher.Load(DataFile(), Problem)) ||
			!TestTrue(What + TEXT("a new game"), Game.NewGame(Choice)))
		{
			return false;
		}
		FRandomPilot Pilot{ uint32(Choice.FirstLevel * 77 + Choice.Players) };
		TArray<uint32> Views;
		int32 First = INDEX_NONE;
		TSharedPtr<FUghReplay> Ended;
		for (int32 Step = 0; Step < 6000 && !Game.IsOver(); ++Step)
		{
			Pilot.Fly(Game, Choice.Players);
			if (Step == 5000)
			{
				Game.MenuKey(UGH_LOGIC_MENU_ESCAPE);   // given up
			}
			Game.Advance(StepSeconds);
			Views.Add(HashOf(Game.GetCurrent()));
			for (const ugh_logic_event& Event : Game.GetEvents())
			{
				First = Event.kind == UGH_LOGIC_EVENT_LEVEL_CAPTION && First == INDEX_NONE ? Step : First;
			}
			Ended = Ended ? Ended : Game.TakeEndedLevel();
		}
		if (!TestTrue(What + TEXT("the game over, its level's replay taken"), Game.IsOver() && Ended.IsValid()))
		{
			return false;
		}
		const ugh_replay_info& Info = Ended->GetInfo();
		TestTrue(What + TEXT("its level, not done, its attempts"), Ended->Level() == Choice.FirstLevel && !Ended->IsDone() &&
			Info.attempts >= 1 && Info.start.lives == 3);
		TestEqual(What + TEXT("from its first caption to the game's end"), First + Info.steps, Views.Num());
		// shared as text, read back, watched
		FString Error;
		const TSharedPtr<FUghReplay> Read = FUghReplay::Read(Ended->Text(), Error);
		if (!TestTrue(What + TEXT("its text read back: ") + Error, Read.IsValid() && Read->Bytes() == Ended->Bytes()) ||
			!TestTrue(What + TEXT("watched"), Watcher.Watch(*Read)))
		{
			return false;
		}
		int32 Same = 0;
		for (int32 Calls = 0; !Watcher.IsOver() && Calls < Info.steps + 10; ++Calls)
		{
			Watcher.Key(0, UGH_LOGIC_KEY_UP, true);   // (ignored: the replay plays its own keys)
			const int32 Before = Watcher.GetWatchedSteps();
			Watcher.Advance(StepSeconds);
			Same += Watcher.GetWatchedSteps() == Before + 1 && Views.IsValidIndex(First + Before) &&
				HashOf(Watcher.GetCurrent()) == Views[First + Before];
		}
		TestEqual(What + TEXT("every step the same view"), Same, Info.steps);
		TestTrue(What + TEXT("then over (the game over in it)"), Watcher.IsOver() && Watcher.GetResult() == UGH_LOGIC_GAME_OVER);
		TestEqual(What + TEXT("the same score"), Watcher.GetCurrent().score, Game.GetCurrent().score);
		TestFalse(What + TEXT("nothing recorded while watching"), Watcher.TakeEndedLevel().IsValid());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRecordMenuTest, "Ugh.Record.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRecordMenuTest::RunTest(const FString&)
{
	/** The test's own clipboard (not the system's). */
	struct FClipboard : IUghClipboard
	{
		FString Text;
		virtual void Copy(const FString& InText) override { Text = InText; }
		virtual FString Paste() override { return Text; }
	} Clipboard;
	char Problem[256] = "";
	ugh_logic* Logic = ugh_logic_create(TCHAR_TO_UTF8(*DataFile()), Problem, sizeof Problem);
	FUghPasswords Passwords;
	FString Error;
	const FString LevelsPath = Assets() / UghJson::LevelsFile;
	TSharedPtr<FJsonObject> Levels;
	if (!TestNotNull(TEXT("the logic"), Logic) ||
		!TestTrue(TEXT("passwords"), UghJson::ReadObject(LevelsPath, Levels, Error) && Passwords.Load(*Levels, LevelsPath, Error)))
	{
		return false;
	}
	const FString Folder = TestFolder();
	FUghReplays Store;
	Store.Open(Folder, Logic, true);
	FCraft Craft;
	Craft.DataHash = ugh_logic_data_hash(Logic);
	Store.OfferBest(Craft.Replay());
	FUghProfile Profile;
	const FUghDisplayOptions Options;
	FUghMenu Menu(Passwords, Profile, Options);
	Menu.SetReplays(&Store, &Clipboard);
	while (Menu.GetRow() != FUghMenu::ERow::Replays)
	{
		Menu.HandleKey(EKeys::Down);
	}
	TestTrue(TEXT("the row Replays after High scores, before Quit"), Menu.HandleKey(EKeys::Down) == FUghMenu::EAction::None &&
		Menu.GetRow() == FUghMenu::ERow::Quit);
	Menu.HandleKey(EKeys::Up);
	Menu.HandleKey(EKeys::Enter);
	TestTrue(TEXT("Enter opens the screen"), Menu.GetScreen() == FUghMenu::EScreen::Replays);
	TestTrue(TEXT("Enter watches the chosen"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Watch &&
		Menu.GetToWatch() && Menu.GetToWatch()->Bytes() == Craft.Bytes());
	Menu.ShowAfterWatching();
	TestTrue(TEXT("after watching the screen again"), Menu.GetScreen() == FUghMenu::EScreen::Replays);
	Menu.HandleKey(EKeys::C);
	TestEqual(TEXT("C copies its text"), Clipboard.Text, Craft.Replay()->Text());
	FCraft Friend = Craft;
	Friend.Level = 0;
	Clipboard.Text = Friend.Replay()->Text();
	Menu.HandleKey(EKeys::V);
	TestTrue(TEXT("V imports the clipboard's: ") + Menu.GetReplaysMenu().GetNotice(), Store.GetEntries().Num() == 2 &&
		Menu.GetReplaysMenu().GetNotice().StartsWith(TEXT("Imported")));
	TestTrue(TEXT("the cursor on it"), Menu.GetReplaysMenu().GetChosen(Store)->Level() == 0);
	Clipboard.Text = TEXT("some other text");
	const int32 Notices = Menu.GetReplaysMenu().GetNoticeCount();
	Menu.HandleKey(EKeys::V);
	TestTrue(TEXT("not a replay: said so"), Store.GetEntries().Num() == 2 && Menu.GetReplaysMenu().GetNoticeCount() == Notices + 1 &&
		Menu.GetReplaysMenu().GetNotice().StartsWith(TEXT("Not imported")));
	Menu.HandleKey(EKeys::Delete);
	TestTrue(TEXT("Delete once asks"), Store.GetEntries().Num() == 2 && Menu.GetReplaysMenu().IsDeleting());
	Menu.HandleKey(EKeys::Delete);
	TestEqual(TEXT("twice deletes"), Store.GetEntries().Num(), 1);
	Menu.HandleKey(EKeys::Delete);
	Menu.HandleKey(EKeys::Down);
	Menu.HandleKey(EKeys::Delete);
	TestEqual(TEXT("another key between: not deleted"), Store.GetEntries().Num(), 1);
	TestTrue(TEXT("Esc back"), Menu.HandleKey(EKeys::Escape) == FUghMenu::EAction::None && Menu.GetScreen() == FUghMenu::EScreen::Title);

	// the level selection: R on a stone with a best replay watches it, on another nothing
	for (int32 Level = 0; Level < 4; ++Level)
	{
		Profile.Scores.SetDone(1, Level);
	}
	Menu.SetIsles(true);
	while (Menu.GetRow() != FUghMenu::ERow::Play)
	{
		Menu.HandleKey(EKeys::Up);
	}
	TestTrue(TEXT("PLAY opens the level selection"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Isles);
	const FUghCameraPose Game = AUghStage::Play(16.0 / 9);
	Menu.OpenIsles(UghMenuView::At(Game, 150, 0), UghMenuView::StoneMiddle(), 150, false);
	while (Menu.GetIsles().GetCursor() != 1)
	{
		Menu.HandleKey(EKeys::Left);
	}
	TestTrue(TEXT("R where there is no best: nothing"), Menu.HandleKey(EKeys::R) == FUghMenu::EAction::None &&
		Menu.GetScreen() == FUghMenu::EScreen::Isles);
	Menu.HandleKey(EKeys::Right);
	TestTrue(TEXT("R on level 3 watches its best"), Menu.HandleKey(EKeys::R) == FUghMenu::EAction::Watch &&
		Menu.GetToWatch() && Menu.GetToWatch()->Level() == 2);
	Menu.ShowAfterWatching();
	TestTrue(TEXT("then the title"), Menu.GetScreen() == FUghMenu::EScreen::Title);

	IFileManager::Get().DeleteDirectory(*Folder, false, true);
	ugh_logic_destroy(Logic);
	return true;
}

#endif
