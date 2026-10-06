// The menu's screens of the settings and of the keys.
#pragma once

#include "CoreMinimal.h"
#include "UghSettingsMenu.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiState;

/**
 * The menu's screen Settings (FUghSettingsMenu) in the title's column: its rows in three parts - the graphics, the
 * sound (the volumes as bars of stones), the game -, the chosen one lit amber with arrows, what it does in a panel
 * below, the keys at the bottom. Only shows FUghUiState.
 */
class SUghSettingsScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghSettingsScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<SWidget> Value(FUghSettingsMenu::ERow Row);
	bool IsChosen(FUghSettingsMenu::ERow Row) const;

	TSharedPtr<const FUghUiState> State;
};

/**
 * The menu's screen Controls (FUghControlsMenu) in the title's column: the pilots' keys as a table (a row a logic key,
 * two cells a pilot; the chosen cell lit, waiting for a key it pulses), the rows Defaults and Back, what the last key
 * did, how the gamepads fly. Only shows FUghUiState.
 */
class SUghControlsScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghControlsScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<SWidget> Cell(int32 Action, int32 Column);
	TSharedRef<SWidget> Gamepads();

	TSharedPtr<const FUghUiState> State;
};
