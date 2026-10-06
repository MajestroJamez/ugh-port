// The profile as automation tests of the editor (Ugh.Settings): saved and read back, the settings' screen, the pilots'
// keys bound in the menu and flying the logic, the high scores.
#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Scalability.h"
#include "UghControls.h"
#include "UghControlsMenu.h"
#include "UghGraphics.h"
#include "UghLogicRecorder.h"
#include "UghProfile.h"
#include "UghSettingsMenu.h"
#include "ugh_logic.h"

namespace
{
	FString ProfileFile(const TCHAR* Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / Name);
	}

	FUghHighScores::FEntry Entry(const TCHAR* Name, uint32 Score)
	{
		return { Name, Score, 4, 1, TEXT("2026-10-06") };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghProfileTest, "Ugh.Settings.SaveLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The profile saved and read back is the same; a missing file is the defaults, a broken one too; the preset a GPU
 * starts with.
 */
bool FUghProfileTest::RunTest(const FString& Parameters)
{
	FUghProfile Saved;
	FUghSettings& Settings = Saved.Settings;
	Settings.Quality = 1;
	Settings.Upscaler = EUghUpscaler::Tsr;
	Settings.FrameGeneration = 2;
	Settings.Resolution = FIntPoint(1600, 900);
	Settings.WindowMode = 2;
	Settings.Volume = 70;
	Settings.Music = 40;
	Settings.Effects = 90;
	Settings.bIntro = false;
	Settings.Keys.Bind({ 0, UGH_LOGIC_KEY_UP, 0 }, EKeys::K);
	Settings.Keys.Bind({ 1, UGH_LOGIC_KEY_FIRE, 1 }, EKeys::RightShift);
	Settings.Keys.Clear({ 0, UGH_LOGIC_KEY_FIRE, 1 });
	Saved.Scores.Insert(1, Entry(TEXT("GROG"), 1200));
	Saved.Scores.Insert(1, Entry(TEXT("ZUG ZUG"), 800));
	Saved.Scores.Insert(2, Entry(TEXT("UGH"), 3150));
	Saved.Scores.SetLastLevel(1, 12);
	Saved.Scores.SetLastLevel(2, 30);
	Saved.Scores.LastName = TEXT("GROG");

	const FString Path = ProfileFile(TEXT("UghProfile.json"));
	FString Error;
	TestTrue(TEXT("saved: ") + Error, Saved.Save(Path, Error));
	FUghProfile Loaded;
	TestTrue(TEXT("read: ") + Error, Loaded.Load(Path, Error));
	TestTrue(TEXT("the same profile read back"), Loaded == Saved);
	TestFalse(TEXT("not the defaults"), Loaded == FUghProfile());

	TestTrue(TEXT("no file: the defaults"), Loaded.Load(ProfileFile(TEXT("None.json")), Error) && Loaded == FUghProfile());
	const FString Broken = ProfileFile(TEXT("Broken.json"));
	FFileHelper::SaveStringToFile(TEXT("{ \"settings\": "), *Broken);
	TestFalse(TEXT("a broken file is refused"), Loaded.Load(Broken, Error));
	TestTrue(TEXT("and the defaults kept"), Loaded == FUghProfile());

	// values out of range keep the defaults, a volume falls to its step, a key of the game is not a pilot's
	const FString Odd = ProfileFile(TEXT("Odd.json"));
	FFileHelper::SaveStringToFile(TEXT("{ \"settings\": { \"quality\": \"Ultra\", \"volume\": 55, \"frameGeneration\": 9, ")
		TEXT("\"keys\": { \"pilot1\": { \"up\": [\"Escape\", \"\"], \"fire\": [\"\", \"NoSuchKey\"] } } } }"), *Odd);
	TestTrue(TEXT("an odd file read"), Loaded.Load(Odd, Error));
	const FUghKeyBindings Defaults = FUghKeyBindings::Defaults();
	TestTrue(TEXT("odd values: the defaults"), Loaded.Settings.Quality == 3 && Loaded.Settings.Volume == 50 &&
		Loaded.Settings.FrameGeneration == 0 && Loaded.Settings.Keys.Get({ 0, UGH_LOGIC_KEY_UP, 0 }) == EKeys::Up &&
		!Loaded.Settings.Keys.Get({ 0, UGH_LOGIC_KEY_FIRE, 0 }).IsValid() &&
		Loaded.Settings.Keys.Get({ 0, UGH_LOGIC_KEY_FIRE, 1 }) == Defaults.Get({ 0, UGH_LOGIC_KEY_FIRE, 1 }));
	IFileManager::Get().DeleteDirectory(*FPaths::AutomationTransientDir(), false, true);

	// without a profile: epic with DLSS (an RTX, integrated or not), low on an integrated GPU, else high
	TestTrue(TEXT("the preset a computer starts with"), FUghSettings::RecommendedQuality(true, false) == 3 &&
		FUghSettings::RecommendedQuality(true, true) == 3 && FUghSettings::RecommendedQuality(false, true) == 0 &&
		FUghSettings::RecommendedQuality(false, false) == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghSettingsMenuTest, "Ugh.Settings.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** The settings' screen: its rows change the settings within what the computer offers. */
bool FUghSettingsMenuTest::RunTest(const FString& Parameters)
{
	using ERow = FUghSettingsMenu::ERow;
	using EResult = FUghSettingsMenu::EResult;
	FUghDisplayOptions Options;   // no DLSS, no frame generation
	Options.Resolutions = { { 1280, 720 }, { 1920, 1080 }, { 2560, 1440 } };
	Options.Current = FIntPoint(1920, 1080);
	FUghSettings Settings;
	FUghSettingsMenu Menu;
	auto Go = [&](ERow Row)
	{
		while (Menu.GetRow() != Row)
		{
			Menu.HandleKey(EKeys::Down, Settings, Options);
		}
	};
	TestTrue(TEXT("Left lowers the quality"), Menu.HandleKey(EKeys::Left, Settings, Options) == EResult::Changed &&
		Settings.Quality == 2);
	Menu.HandleKey(EKeys::Right, Settings, Options);
	TestTrue(TEXT("epic is the highest"), Menu.HandleKey(EKeys::Right, Settings, Options) == EResult::None);
	Go(ERow::Upscaler);
	TestEqual(TEXT("without DLSS: FSR"), FUghSettingsMenu::ValueOf(ERow::Upscaler, Settings, Options), FString(TEXT("FSR")));
	Menu.HandleKey(EKeys::Right, Settings, Options);
	TestTrue(TEXT("then TSR"), Settings.Upscaler == EUghUpscaler::Tsr);
	Menu.HandleKey(EKeys::Right, Settings, Options);
	TestTrue(TEXT("then FSR again"), Settings.Upscaler == EUghUpscaler::Fsr);
	Go(ERow::FrameGeneration);
	TestTrue(TEXT("no frame generation to choose"), Menu.HandleKey(EKeys::Right, Settings, Options) == EResult::None);
	Go(ERow::Resolution);
	Menu.HandleKey(EKeys::Right, Settings, Options);
	TestEqual(TEXT("the next resolution"), Settings.Resolution, FIntPoint(2560, 1440));
	Menu.HandleKey(EKeys::Right, Settings, Options);
	TestEqual(TEXT("round to the first"), Settings.Resolution, FIntPoint(1280, 720));
	Go(ERow::Window);
	Menu.HandleKey(EKeys::Right, Settings, Options);
	TestEqual(TEXT("borderless to a window"), Settings.WindowMode, 2);
	Go(ERow::Music);
	for (int32 Step = 0; Step < 12; ++Step)
	{
		Menu.HandleKey(EKeys::Left, Settings, Options);
	}
	TestTrue(TEXT("the music silent at the least"), Settings.Music == 0 && Settings.Volume == 100);
	Go(ERow::Intro);
	Menu.HandleKey(EKeys::Enter, Settings, Options);
	TestFalse(TEXT("Enter turns the flight off"), Settings.bIntro);
	Go(ERow::Controls);
	TestTrue(TEXT("Enter on Controls opens them"), Menu.HandleKey(EKeys::Enter, Settings, Options) == EResult::Controls);
	TestTrue(TEXT("a gamepad's B goes back"), Menu.HandleKey(FUghControls::BackKey(), Settings, Options) == EResult::Back);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghKeysTest, "Ugh.Settings.Keys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** A key bound in the menu's screen Controls flies the logic: swapped where it was, refused for the game's own. */
bool FUghKeysTest::RunTest(const FString& Parameters)
{
	using EResult = FUghControlsMenu::EResult;
	FUghKeyBindings Keys = FUghKeyBindings::Defaults();
	FUghControlsMenu Menu;
	Menu.Open();
	Menu.HandleKey(EKeys::Enter, Keys);
	TestTrue(TEXT("Enter waits for a key"), Menu.IsCapturing());
	TestTrue(TEXT("K for pilot 1's up"), Menu.HandleKey(EKeys::K, Keys) == EResult::Changed && !Menu.IsCapturing());

	FUghControls Controls(Keys);
	FUghLogicRecorder Logic;
	const FInputDeviceId Keyboard = FInputDeviceId::CreateFromInternalId(0);
	Controls.Handle(Logic, EKeys::K, IE_Pressed, Keyboard);
	Controls.Handle(Logic, EKeys::Up, IE_Pressed, Keyboard);
	TestEqual(TEXT("K flies up, the arrow nothing (but a key)"), Logic.Take(),
		FString(TEXT("key 0 up on, menu other, menu other")));

	// a key of the other pilot: the two swap
	Menu.HandleKey(EKeys::Down, Keys);
	Menu.HandleKey(EKeys::Enter, Keys);
	TestTrue(TEXT("W for pilot 1's down"), Menu.HandleKey(EKeys::W, Keys) == EResult::Changed);
	TestEqual(TEXT("pilot 2's up has the arrow down"), Keys.Get({ 1, UGH_LOGIC_KEY_UP, 0 }), EKeys::Down);
	TestFalse(TEXT("and it is said"), Menu.GetNotice().IsEmpty());
	Controls.Handle(Logic, EKeys::W, IE_Pressed, Keyboard);
	Controls.Handle(Logic, EKeys::Down, IE_Pressed, Keyboard);
	TestEqual(TEXT("swapped in the logic too"), Logic.Take(),
		FString(TEXT("key 0 down on, menu other, key 1 up on, menu other")));

	// the game's own key refused, Esc leaves it
	Menu.HandleKey(EKeys::Enter, Keys);
	TestTrue(TEXT("P refused"), Menu.HandleKey(EKeys::P, Keys) == EResult::None && Menu.IsCapturing());
	TestTrue(TEXT("Esc leaves it"), Menu.HandleKey(EKeys::Escape, Keys) == EResult::None && !Menu.IsCapturing());
	TestEqual(TEXT("as it was"), Keys.Get({ 0, UGH_LOGIC_KEY_DOWN, 0 }), EKeys::W);
	TestTrue(TEXT("Backspace: no key"), Menu.HandleKey(EKeys::BackSpace, Keys) == EResult::Changed &&
		!Keys.Get({ 0, UGH_LOGIC_KEY_DOWN, 0 }).IsValid());

	// the second key of pilot 2's fire (the fourth column): Space moves there from pilot 1
	Menu.HandleKey(EKeys::Left, Keys);
	Menu.HandleKey(EKeys::Down, Keys);
	Menu.HandleKey(EKeys::Down, Keys);
	Menu.HandleKey(EKeys::Down, Keys);
	TestTrue(TEXT("the cell of pilot 2's second fire"), Menu.GetPlace() == FUghKeyBindings::FPlace{ 1, UGH_LOGIC_KEY_FIRE, 1 });
	Menu.HandleKey(EKeys::Enter, Keys);
	Menu.HandleKey(EKeys::SpaceBar, Keys);
	TestFalse(TEXT("Space is not pilot 1's any more"), Keys.Get({ 0, UGH_LOGIC_KEY_FIRE, 1 }).IsValid());
	Controls.Handle(Logic, EKeys::SpaceBar, IE_Pressed, Keyboard);
	TestEqual(TEXT("Space fires for pilot 2"), Logic.Take(), FString(TEXT("key 1 fire on, menu other")));

	Menu.HandleKey(EKeys::Down, Keys);
	TestTrue(TEXT("the defaults again"), Menu.HandleKey(EKeys::Enter, Keys) == EResult::Changed &&
		Keys == FUghKeyBindings::Defaults());
	Menu.HandleKey(EKeys::Down, Keys);
	TestTrue(TEXT("Enter on Back goes back"), Menu.HandleKey(EKeys::Enter, Keys) == EResult::Back);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghHighScoresTest, "Ugh.Settings.HighScores",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** The ten best of each mode: sorted, a new score below the same one reached earlier, kept in the profile. */
bool FUghHighScoresTest::RunTest(const FString& Parameters)
{
	FUghHighScores Scores;
	TestEqual(TEXT("no points, no place"), Scores.RankOf(1, 0), INDEX_NONE);
	TestEqual(TEXT("an empty table takes any"), Scores.RankOf(1, 5), 0);
	const uint32 Points[] = { 700, 1200, 300, 900, 100, 1100, 500, 1000, 200, 800, 400, 600 };
	for (const uint32 Score : Points)
	{
		Scores.Insert(1, Entry(*FString::Printf(TEXT("P%u"), Score), Score));
	}
	const TArray<FUghHighScores::FEntry>& Table = Scores.Table(1);
	TestEqual(TEXT("ten kept"), Table.Num(), FUghHighScores::Size);
	TestTrue(TEXT("the best first"), Table[0].Score == 1200 && Table.Last().Score == 300);
	bool bSorted = true;
	for (int32 Place = 1; Place < Table.Num(); ++Place)
	{
		bSorted = bSorted && Table[Place - 1].Score >= Table[Place].Score;
	}
	TestTrue(TEXT("sorted"), bSorted);
	TestEqual(TEXT("the tenth's score does not get in"), Scores.RankOf(1, 300), INDEX_NONE);
	TestEqual(TEXT("a point more does"), Scores.RankOf(1, 301), 9);
	TestEqual(TEXT("below the same score reached earlier"), Scores.Insert(1, Entry(TEXT("LATE"), 1000)), 3);
	TestEqual(TEXT("the earlier one stays above"), Table[2].Name, FString(TEXT("P1000")));
	TestTrue(TEXT("the team's table apart"), Scores.Table(2).IsEmpty() && Scores.RankOf(2, 1) == 0);
	Scores.Insert(2, Entry(TEXT("  A VERY LONG NAME  "), 50));
	TestEqual(TEXT("a name trimmed and cut"), Scores.Table(2)[0].Name, FString(TEXT("A VERY LON")));

	FUghProfile Saved;
	Saved.Scores = Scores;
	const FString Path = ProfileFile(TEXT("Scores.json"));
	FString Error;
	FUghProfile Loaded;
	TestTrue(TEXT("kept in the profile"), Saved.Save(Path, Error) && Loaded.Load(Path, Error) && Loaded.Scores == Scores);
	IFileManager::Get().DeleteDirectory(*FPaths::AutomationTransientDir(), false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghQualityTest, "Ugh.Settings.Quality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * A quality preset stays when the engine applies its user settings again (Alt+Enter toggling the full screen does:
 * a game at low went on at epic): the scalability groups and the preset's variables (low: half resolution under FSR,
 * dynamic resolution, no halo, no fire shadows).
 */
bool FUghQualityTest::RunTest(const FString& Parameters)
{
	UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!TestNotNull(TEXT("the engine's user settings"), User))
	{
		return false;
	}
	const Scalability::FQualityLevels Before = Scalability::GetQualityLevels();
	auto Variable = [](const TCHAR* Name)
	{
		IConsoleVariable* Found = IConsoleManager::Get().FindConsoleVariable(Name);
		return Found ? Found->GetString() : FString();
	};
	for (int32 Quality = 0; Quality < FUghSettings::QualityLevels; ++Quality)
	{
		UghGraphics::ApplyQuality(Quality, nullptr);
		User->ApplyNonResolutionSettings();   // what Alt+Enter does
		const Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
		const FString What = FUghSettings::QualityName(Quality);
		TestEqual(What + TEXT(": the effects' scalability kept"), Levels.EffectsQuality, Quality);
		TestEqual(What + TEXT(": the shadows' scalability kept"), Levels.ShadowQuality, FMath::Max(Quality, 1));
		TestEqual(What + TEXT(": the global illumination's scalability kept"), Levels.GlobalIlluminationQuality, Quality);
		for (const UghGraphics::FVariable& Each : UghGraphics::Variables())
		{
			// (r.ScreenPercentage: FSR sets it by code from its own mode)
			if (IConsoleManager::Get().FindConsoleVariable(Each.Name) && FCString::Strcmp(Each.Name, TEXT("r.ScreenPercentage")))
			{
				TestEqual(What + TEXT(": ") + Each.Name, FCString::Atof(*Variable(Each.Name)),
					FCString::Atof(Each.Values[Quality]), 0.01f);
			}
		}
	}
	UghGraphics::ApplyQuality(FUghSettings::QualityLevels - 1, nullptr);
	Scalability::SetQualityLevels(Before);
	return true;
}

#endif
