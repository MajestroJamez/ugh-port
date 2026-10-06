// The menu's screen of the pilots' keys.
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UghKeyBindings.h"

/**
 * The screen Controls of the menu (FUghMenu): the pilots' keys as a table - a row for each of the logic's keys (up,
 * down, left, right, fire), a column for each of the two keys of pilot 1 and of pilot 2 -, then the rows Defaults and
 * Back. The arrows choose a cell, Enter waits for the next key and binds it there (a key bound elsewhere swaps places
 * with the key that was there; the game's own keys are refused; Esc or a gamepad's B leave it as it was), Backspace
 * clears the cell, Enter on Defaults puts the remake's keys back; Esc, B or Enter on Back go back. The gamepads are
 * not bound here (FUghControls). Only the presses count; UghUi shows it, and what the last key did (GetNotice).
 */
class FUghControlsMenu
{
public:
	static constexpr int32 DefaultsRow = FUghKeyBindings::Actions, BackRow = DefaultsRow + 1, RowCount = BackRow + 1;
	static constexpr int32 Columns = FUghKeyBindings::Pilots * FUghKeyBindings::Slots;

	/** What a key did. */
	enum class EResult : uint8 { None, Changed, Back };

	EResult HandleKey(const FKey& Key, FUghKeyBindings& Bindings);
	/** Opened from the settings: the first cell, nothing said. */
	void Open();

	int32 GetRow() const { return Row; }
	int32 GetColumn() const { return Column; }
	/** The cell chosen (on a row of a key). */
	FUghKeyBindings::FPlace GetPlace() const;
	/** Waiting for the key to bind. */
	bool IsCapturing() const { return bCapturing; }
	/** What the last key did (bound, swapped, refused), empty for nothing. */
	const FString& GetNotice() const { return Notice; }

	/** A cell as the screen names it: "Pilot 1 · Fire". */
	static FString NameOf(const FUghKeyBindings::FPlace& Place);

private:
	EResult Capture(const FKey& Key, FUghKeyBindings& Bindings);

	int32 Row = 0;
	int32 Column = 0;
	bool bCapturing = false;
	FString Notice;
};
