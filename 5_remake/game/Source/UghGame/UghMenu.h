// The menu before a game.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UghControlsMenu.h"
#include "UghNameEntry.h"
#include "UghSettingsMenu.h"

class FUghPasswords;
struct FUghProfile;

/** What a new game starts with: chosen in the menu. */
struct FUghGameChoice
{
	int32 Players = 1;      // 2: the team mode
	int32 Difficulty = 1;   // 0 easy, 1 medium, 2 hard
	int32 FirstLevel = 0;   // from 0 in the order of the mode

	bool operator==(const FUghGameChoice& Other) const = default;
};

/** How a game ended, for the menu. */
struct FUghGameEnd
{
	FUghGameChoice Choice;   // what it started with
	int32 Level = 0;         // where it ended, from 0
	uint32 Score = 0;
	bool bAllDone = false;   // every level done, else game over
};

/**
 * The menu before a game. The title screen: one player or the team, the difficulty, a level's password (the original's
 * menu and its F2), play, the settings, the high scores, quit. Up and Down choose a row, Left and Right change the mode
 * or the difficulty (on the password Right fills in the password of the level the mode's last game got to, Left
 * clears it), letters and digits type the password (on any row), Backspace deletes, Enter plays (on Settings, High
 * scores, Quit: opens it, quits), Esc quits. A password the mode does not know stops Enter. The screens Settings
 * (FUghSettingsMenu, with Controls, FUghControlsMenu) and High scores (any key goes back) change the profile
 * (FUghProfile). After a game it shows how the game ended until a key (which does nothing else); a score among the
 * mode's ten best first asks for a name (FUghNameEntry), then shows the high scores with it. Only the presses count
 * (a gamepad's buttons as the keys FUghControls::MenuKeyOf gives; its B goes back); the menu does not draw itself
 * (UghUi does).
 */
class FUghMenu
{
public:
	enum class ERow : uint8 { Players, Difficulty, Password, Play, Settings, Scores, Quit };
	static constexpr int32 RowCount = 7, DifficultyCount = 3, MaxPasswordLength = 24;

	enum class EScreen : uint8 { Title, Settings, Controls, Scores };

	/** What the game mode does after a key: play, quit, apply the settings and save the profile (Save). */
	enum class EAction : uint8 { None, Play, Quit, Save };

	FUghMenu(const FUghPasswords& InPasswords, FUghProfile& InProfile, const FUghDisplayOptions& InOptions)
		: Passwords(InPasswords), Profile(InProfile), Options(InOptions) {}

	EAction HandleKey(const FKey& Key);

	/** The game the menu would start (the first level when the password is unknown). */
	FUghGameChoice GetChoice() const;

	EScreen GetScreen() const { return Screen; }
	ERow GetRow() const { return Row; }
	const FString& GetPassword() const { return Password; }
	/** The password is empty or one of the mode's. */
	bool IsPasswordKnown() const { return Password.IsEmpty() || PasswordLevel() != INDEX_NONE; }
	const FUghSettingsMenu& GetSettingsMenu() const { return SettingsMenu; }
	const FUghControlsMenu& GetControlsMenu() const { return ControlsMenu; }

	/** A game ended: shown until a key (a high score: until its name is typed); then it is the last game. */
	void ShowEnd(const FUghGameEnd& End);
	bool IsShowingEnd() const { return bShowingEnd; }
	const TOptional<FUghGameEnd>& GetLastGame() const { return LastGame; }
	/** The name being typed for the high scores, and the place the score takes. */
	const TOptional<FUghNameEntry>& GetNameEntry() const { return NameEntry; }
	int32 GetNewRank() const { return NewRank; }
	/** The high scores' new entry to light (the mode's players, its place), shown with the screen High scores. */
	const TOptional<TPair<int32, int32>>& GetHighlight() const { return Highlight; }

	/** Names for the screen. */
	static const TCHAR* DifficultyName(int32 Difficulty);
	/** The character a key types in a password (A .. Z, 0 .. 9), 0 for any other key; and the key of one. */
	static TCHAR CharOf(const FKey& Key);
	static FKey KeyOf(TCHAR Char);

private:
	EAction HandleTitleKey(const FKey& Key);
	EAction HandleEndKey(const FKey& Key);
	int32 PasswordLevel() const;
	void Change(int32 Direction);

	const FUghPasswords& Passwords;
	FUghProfile& Profile;
	const FUghDisplayOptions& Options;
	EScreen Screen = EScreen::Title;
	ERow Row = ERow::Players;
	int32 Players = 1;
	int32 Difficulty = 1;
	FString Password;
	FUghSettingsMenu SettingsMenu;
	FUghControlsMenu ControlsMenu;
	TOptional<FUghGameEnd> LastGame;
	bool bShowingEnd = false;
	TOptional<FUghNameEntry> NameEntry;
	int32 NewRank = INDEX_NONE;
	TOptional<TPair<int32, int32>> Highlight;
};
