// The title screen.
#pragma once

#include "CoreMinimal.h"
#include "UghMenu.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiState;

/**
 * The title screen over the sea stack (UghUi): the logo carved in stone, the menu's rows (FUghMenu: the mode, the
 * difficulty, the password field - offering the password of the level the mode's last game got to -, play, the
 * settings, the high scores, quit; the chosen one lit amber), the last game, the keys in a line; the menu's other
 * screens in its place (SUghSettingsScreen, SUghControlsScreen, SUghScoresScreen); after a game a card of how it ended
 * until a key, with a high score's name being typed. Only shows FUghUiState: the keys go to FUghMenu.
 */
class SUghMenuScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghMenuScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<SWidget> Title();
	/** A row of a setting: its label, its value between arrows to change it while chosen. */
	TSharedRef<SWidget> Setting(FUghMenu::ERow Row, const FString& Label, const TSharedRef<SWidget>& Value);
	TSharedRef<SWidget> Password();
	TSharedRef<SWidget> Play();
	TSharedRef<SWidget> Keys();
	TSharedRef<SWidget> LastGame();
	TSharedRef<SWidget> EndCard();
	/** The name of a high score being typed on the card: a stone a letter, the cursor's lit. */
	TSharedRef<SWidget> NameEntry();
	bool IsChosen(FUghMenu::ERow Row) const;
	bool IsShowing(FUghMenu::EScreen Screen) const;

	TSharedPtr<const FUghUiState> State;
};
