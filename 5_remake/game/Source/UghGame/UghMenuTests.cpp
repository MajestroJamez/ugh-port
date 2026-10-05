// The menu as an automation test of the editor (Ugh.Menu): its keys, the passwords of both modes.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghJson.h"
#include "UghMenu.h"
#include "UghPasswords.h"

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

	FUghMenu Menu(Passwords);
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
	TestTrue(TEXT("Enter on Quit quits"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Quit);
	Menu.HandleKey(EKeys::Down);
	TestTrue(TEXT("Down from the last row goes round"), Menu.GetRow() == FUghMenu::ERow::Players);
	Menu.HandleKey(EKeys::Up);
	TestTrue(TEXT("Up from the first row goes round"), Menu.GetRow() == FUghMenu::ERow::Quit);
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
	Menu.ShowEnd({ Chosen, 3, 1200, false });
	TestTrue(TEXT("the end of a game shown"), Menu.IsShowingEnd() && Menu.GetLastGame().IsSet());
	TestTrue(TEXT("a key closes it and does nothing else"),
		Menu.HandleKey(EKeys::Escape) == FUghMenu::EAction::None && !Menu.IsShowingEnd() && Menu.GetChoice() == Chosen);
	TestEqual(TEXT("the last game stays"), Menu.GetLastGame()->Score, 1200u);
	TestTrue(TEXT("Esc quits"), Menu.HandleKey(EKeys::Escape) == FUghMenu::EAction::Quit);
	return true;
}

#endif
