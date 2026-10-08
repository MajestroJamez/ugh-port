// What the screen shows of the game.
#pragma once

#include "CoreMinimal.h"
#include "UghHighScores.h"
#include "UghMenu.h"
#include "UghSettings.h"
#include "UghUiStyle.h"

/** A score earned, rising where it was earned. */
struct FUghUiPopup
{
	FVector2D Where;   // a part of the view across and down, 0 .. 1
	int32 Points = 0;
	float Age = 0;     // a part of its time, 0 .. 1
};

/** The warning over a copter flying fast enough to crash (UghWarning). */
struct FUghUiWarning
{
	FVector2D Where;     // over the copter: a part of the view across and down, 0 .. 1
	int32 Loudness = 0;  // 1 .. UghWarning::Loudest
	double Age = 0;      // seconds since it began: its blinking
};

/** A stone of the level selection on the screen: its number shown on it. */
struct FUghUiIsle
{
	FVector2D Where;   // a part of the view across and down, 0 .. 1
	float Size = 1;    // how big its number shows (by how near it is), 0 .. 1
	int32 Level = 0;   // from 0
	EUghIsle State = EUghIsle::Locked;
	bool bCursor = false;
};

/**
 * What the screen (UghUi) shows of the game, taken every frame by AUghHud: the menu and its screens (the settings, the
 * keys, the high scores), the status of the play, its caption, the help, a setting just changed, the scores rising. The
 * widgets only read it.
 */
struct FUghUiState
{
	enum class EScreen : uint8 { Problem, Menu, Play };
	EScreen Screen = EScreen::Menu;
	FString Problem;   // why there is no game
	/** Seconds of the real clock: what moves on the screen (a caret blinks, a hint pulses). */
	double Time = 0;

	// the menu (FUghMenu)
	FUghMenu::ERow Row = FUghMenu::ERow::Players;
	FUghGameChoice Choice;
	int32 Levels = 0;   // of the chosen mode
	FString Password;
	bool bPasswordKnown = true;
	bool bShowingEnd = false;
	TOptional<FUghGameEnd> LastGame;
	int32 Volume = 100;   // percent
	/** The level the mode's last game got to (-1 none) and its password, of one player and of the team. */
	int32 LastLevels[2] = { -1, -1 };
	FString LastPasswords[2];

	// the menu's other screens
	FUghMenu::EScreen MenuScreen = FUghMenu::EScreen::Title;
	FUghSettings Settings;   // the profile's
	FUghDisplayOptions Options;
	FUghSettingsMenu::ERow SettingsRow = FUghSettingsMenu::ERow::Quality;
	int32 ControlsRow = 0, ControlsColumn = 0;
	bool bCapturing = false;
	FString ControlsNotice;
	FUghHighScores Scores;
	TOptional<TPair<int32, int32>> Highlight;   // the high scores' new entry: the mode's players, its place
	TOptional<FUghNameEntry> NameEntry;   // a high score's name being typed after a game
	int32 NewRank = INDEX_NONE;

	// the level selection (FUghIsles)
	TArray<FUghUiIsle> Isles;        // the stones in view, far ones first
	float IslesShown = 0;            // how much of its screen shows (not in the flights' ends), 0 .. 1
	int32 IslesCursor = 0;           // the cursor's level, from 0
	EUghIsle IslesCursorState = EUghIsle::Locked;
	FString IslesPassword;           // of the cursor's level
	int32 IslesDone = 0, IslesCount = 0;
	FString IslesNotice;             // why Enter did nothing
	double IslesNoticeAge = 1e9;

	// the play (the logic's view)
	int32 Phase = 0;   // UGH_LOGIC_PHASE_...
	int32 Players = 1, Level = 0, Lives = 0, Multiplier = 1;
	uint32 Score = 0;
	float Energy = 1;            // 0 .. 1
	float Shown = 0;             // how much of the play shows (its fades), 0 .. 1
	double CaptionAge = 0;       // seconds since the caption came up
	FString LevelPassword;       // of the level shown
	float Help = 0;              // how much the help shows, 0 .. 1
	FString Upscaler;
	/** A setting just changed (the volume, the upscaler): what, how long ago (seconds), the volume's bar (-1 none). */
	FString Notice;
	double NoticeAge = 1e9;
	float NoticeLevel = -1;
	TArray<FUghUiPopup> Popups;
	TArray<FUghUiWarning> Warnings;

	/** The pictures of the screen (UghStoneArt). */
	TSharedPtr<const UghUiStyle::FPictures> Pictures;
};
