// The menu as an automation test of the editor (Ugh.Menu): its keys, the passwords of both modes, its screens, a high
// score's name after a game, the last level's password.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghControls.h"
#include "UghJson.h"
#include "UghMenu.h"
#include "UghPasswords.h"
#include "UghProfile.h"

namespace
{
	/** Types `Text` into the menu, a key a character. */
	void Type(FUghMenu& Menu, const FString& Text)
	{
		for (const TCHAR Char : Text)
		{
			Menu.HandleKey(FUghMenu::KeyOf(Char));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghMenuTest, "Ugh.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghMenuTest::RunTest(const FString& Parameters)
{
	FUghPasswords Passwords;
	FString Error;
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	const FString Path = Assets / UghJson::LevelsFile;
	TSharedPtr<FJsonObject> Levels;
	const bool bLoaded = UghJson::ReadObject(Path, Levels, Error) && Passwords.Load(*Levels, Path, Error);
	if (!TestTrue(TEXT("passwords: ") + Error, bLoaded))
	{
		return false;
	}
	TestEqual(TEXT("one player's levels"), Passwords.LevelCount(1), 69);
	TestEqual(TEXT("the team's levels"), Passwords.LevelCount(2), 81);

	FUghProfile Profile;
	const FUghDisplayOptions Options;
	FUghMenu Menu(Passwords, Profile, Options);
	TestTrue(TEXT("one player on medium from the first level"), Menu.GetChoice() == FUghGameChoice{ 1, 1, 0 });
	TestTrue(TEXT("Enter plays"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Play);

	Menu.HandleKey(EKeys::Right);
	Menu.HandleKey(EKeys::Down);
	Menu.HandleKey(EKeys::Right);
	Menu.HandleKey(EKeys::Right);
	TestTrue(TEXT("the team on hard"), Menu.GetChoice() == FUghGameChoice{ 2, 2, 0 });
	Menu.HandleKey(EKeys::Down);
	TestTrue(TEXT("the third row"), Menu.GetRow() == FUghMenu::ERow::Password);
	Menu.HandleKey(EKeys::Down);
	TestTrue(TEXT("the row Play"), Menu.GetRow() == FUghMenu::ERow::Play);
	TestTrue(TEXT("Enter on Play plays"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Play);
	Menu.HandleKey(EKeys::Down);
	TestTrue(TEXT("Enter on Settings opens them"),
		Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::None && Menu.GetScreen() == FUghMenu::EScreen::Settings);
	TestTrue(TEXT("a change of a setting saves"), Menu.HandleKey(EKeys::Left) == FUghMenu::EAction::Save &&
		Profile.Settings.Quality == 2);
	Menu.HandleKey(EKeys::Escape);
	TestTrue(TEXT("Esc back to the title"), Menu.GetScreen() == FUghMenu::EScreen::Title && Menu.GetRow() == FUghMenu::ERow::Settings);
	Menu.HandleKey(EKeys::Down);
	Menu.HandleKey(EKeys::Enter);
	TestTrue(TEXT("Enter on High scores shows them"), Menu.GetScreen() == FUghMenu::EScreen::Scores);
	Menu.HandleKey(EKeys::Gamepad_FaceButton_Right);
	TestTrue(TEXT("any key goes back"), Menu.GetScreen() == FUghMenu::EScreen::Title);
	Menu.HandleKey(EKeys::Down);
	TestTrue(TEXT("Enter on Quit quits"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Quit);
	Menu.HandleKey(EKeys::Down);
	TestTrue(TEXT("Down from the last row goes round"), Menu.GetRow() == FUghMenu::ERow::Players);
	Menu.HandleKey(EKeys::Up);
	TestTrue(TEXT("Up from the first row goes round"), Menu.GetRow() == FUghMenu::ERow::Quit);
	TestTrue(TEXT("a gamepad's B quits nothing"), Menu.HandleKey(FUghControls::BackKey()) == FUghMenu::EAction::None);
	Menu.HandleKey(EKeys::Down);

	Type(Menu, Passwords.Get(2, 40));
	TestEqual(TEXT("a team password"), Menu.GetChoice().FirstLevel, 40);
	Menu.HandleKey(EKeys::Left);   // one player: the team's password is not its
	TestFalse(TEXT("a password of the other mode"), Menu.IsPasswordKnown());
	TestTrue(TEXT("an unknown password stops Enter"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::None);
	TestEqual(TEXT("and starts nowhere else"), Menu.GetChoice().FirstLevel, 0);

	while (!Menu.GetPassword().IsEmpty())
	{
		Menu.HandleKey(EKeys::BackSpace);
	}
	const int32 Digits = Passwords.Find(1, TEXT("1983"));
	Type(Menu, TEXT("1983"));
	TestTrue(TEXT("a password of digits"), Digits != INDEX_NONE && Menu.GetChoice().FirstLevel == Digits);
	const FUghGameChoice Chosen = Menu.GetChoice();
	Menu.ShowEnd({ Chosen, 3, 0, false });
	TestTrue(TEXT("the end of a game shown"), Menu.IsShowingEnd() && Menu.GetLastGame().IsSet() && !Menu.GetNameEntry());
	TestTrue(TEXT("a key closes it and does nothing else"),
		Menu.HandleKey(EKeys::Escape) == FUghMenu::EAction::None && !Menu.IsShowingEnd() && Menu.GetChoice() == Chosen);
	TestEqual(TEXT("the last game stays"), Menu.GetLastGame()->Level, 3);

	// a high score: its name typed with the keys and a gamepad's letters, carved, shown lit in the high scores
	Profile.Scores.LastName = TEXT("GROG");
	Menu.ShowEnd({ Chosen, 7, 1200, false });
	TestTrue(TEXT("a high score asks for a name"), Menu.GetNameEntry().IsSet() && Menu.GetNewRank() == 0);
	TestEqual(TEXT("the last name to begin with"), Menu.GetNameEntry()->GetName(), FString(TEXT("GROG")));
	Menu.HandleKey(EKeys::BackSpace);
	Type(Menu, TEXT("K"));
	for (const FKey& Button : { EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Up,
		EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_DPad_Up })
	{
		Menu.HandleKey(FUghControls::MenuKeyOf(Button));
	}
	TestEqual(TEXT("typed and turned"), Menu.GetNameEntry()->GetName(), FString(TEXT("GROKA")));
	TestTrue(TEXT("a gamepad's A carves it"),
		Menu.HandleKey(FUghControls::MenuKeyOf(EKeys::Gamepad_FaceButton_Bottom)) == FUghMenu::EAction::Save);
	const FUghHighScores::FEntry& Best = Profile.Scores.Table(1)[0];
	TestTrue(TEXT("the first of the high scores"), Best.Name == TEXT("GROKA") && Best.Score == 1200 && Best.Level == 7);
	TestTrue(TEXT("shown lit"), Menu.GetScreen() == FUghMenu::EScreen::Scores && !Menu.IsShowingEnd() &&
		Menu.GetHighlight() == MakeTuple(1, 0));
	Menu.HandleKey(EKeys::Enter);

	// Right on the password: the one of the level the mode's last game got to
	Profile.Scores.SetLastLevel(1, 12);
	while (Menu.GetRow() != FUghMenu::ERow::Password)
	{
		Menu.HandleKey(EKeys::Down);
	}
	Menu.HandleKey(EKeys::Right);
	TestEqual(TEXT("the last level's password"), Menu.GetChoice().FirstLevel, 12);
	Menu.HandleKey(EKeys::Left);
	TestTrue(TEXT("Left clears it"), Menu.GetPassword().IsEmpty());
	TestTrue(TEXT("Esc quits"), Menu.HandleKey(EKeys::Escape) == FUghMenu::EAction::Quit);
	return true;
}

#endif
