// A name typed for the high scores.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UghHighScores.h"

/**
 * A name typed after a game that reached the high scores (FUghMenu), a box a letter: with the keyboard - letters,
 * digits and Space at the cursor, Backspace deletes the letter at the cursor (after the name its last) - or with a
 * gamepad as an arcade machine's - Up and Down turn the letter at the cursor (A .. Z, 0 .. 9, a space), Left and Right
 * move it, B deletes. Enter (a gamepad's A) or Esc carve it.
 */
class FUghNameEntry
{
public:
	static constexpr int32 MaxLength = FUghHighScores::MaxName;

	/** Starts with `Name` (the last one typed), the cursor after it. */
	explicit FUghNameEntry(const FString& Name);

	/** True when the name is done. */
	bool HandleKey(const FKey& Key);

	const FString& GetName() const { return Name; }
	/** Where the next character goes (0 .. the name's length, below MaxLength). */
	int32 GetCursor() const { return Cursor; }
	/** The name carved: trimmed, a cave dweller's when empty. */
	FString GetCarved() const;

private:
	/** The character at the cursor `Steps` on in the arcade's order. */
	void Turn(int32 Steps);
	void Type(TCHAR Char);

	FString Name;
	int32 Cursor = 0;
};
