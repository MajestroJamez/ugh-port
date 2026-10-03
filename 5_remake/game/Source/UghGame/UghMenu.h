// The menu before a game.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class FUghPasswords;

/** What a new game starts with: chosen in the menu. */
struct FUghGameChoice
{
	int32 Players = 1;      // 2: the team mode
	int32 Difficulty = 1;   // 0 easy, 1 medium, 2 hard
	int32 FirstLevel = 0;   // from 0 in the order of the mode

	bool operator==(const FUghGameChoice& Other) const = default;
};

/**
 * The menu before a game: one player or the team, the difficulty and a level's password (the original's menu and its
 * F2). Up and Down choose a row, Left and Right change the mode or the difficulty, letters and digits type the
 * password (on any row), Backspace deletes, Enter plays, Esc quits. A password the mode does not know stops Enter.
 * Only the presses count; the menu does not draw itself (the HUD does).
 */
class FUghMenu
{
public:
	enum class ERow : uint8 { Players, Difficulty, Password };
	static constexpr int32 RowCount = 3, DifficultyCount = 3, MaxPasswordLength = 24;

	/** What the game mode does after a key. */
	enum class EAction : uint8 { None, Play, Quit };

	explicit FUghMenu(const FUghPasswords& InPasswords) : Passwords(InPasswords) {}

	EAction HandleKey(const FKey& Key);

	/** The game the menu would start (the first level when the password is unknown). */
	FUghGameChoice GetChoice() const;

	ERow GetRow() const { return Row; }
	const FString& GetPassword() const { return Password; }
	/** The password is empty or one of the mode's. */
	bool IsPasswordKnown() const { return Password.IsEmpty() || PasswordLevel() != INDEX_NONE; }

	/** Names for the HUD. */
	static const TCHAR* DifficultyName(int32 Difficulty);
	/** The character a key types in a password (A .. Z, 0 .. 9), 0 for any other key; and the key of one. */
	static TCHAR CharOf(const FKey& Key);
	static FKey KeyOf(TCHAR Char);
	static const TCHAR* KeysHelp()
	{
		return TEXT("Up/Down: choose   Left/Right: change   type a password   Enter: play   Esc: quit");
	}

private:
	int32 PasswordLevel() const;
	void Change(int32 Direction);

	const FUghPasswords& Passwords;
	ERow Row = ERow::Players;
	int32 Players = 1;
	int32 Difficulty = 1;
	FString Password;
};
