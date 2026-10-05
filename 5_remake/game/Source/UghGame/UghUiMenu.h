// The title screen.
#pragma once

#include "CoreMinimal.h"
#include "UghMenu.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiState;

/**
 * The title screen over the sea stack (UghUi): the logo carved in stone, the menu's rows (FUghMenu: the mode, the
 * difficulty, the password field, play, quit; the chosen one lit amber), the last game, the keys in a line; after a
 * game a card of how it ended until a key. Only shows FUghUiState: the keys go to FUghMenu.
 */
class SUghMenuScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghMenuScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	/** A row of a setting: its label, its value between arrows to change it while chosen. */
	TSharedRef<SWidget> Setting(FUghMenu::ERow Row, const FString& Label, const TSharedRef<SWidget>& Value);
	TSharedRef<SWidget> Password();
	TSharedRef<SWidget> Play();
	TSharedRef<SWidget> Quit();
	TSharedRef<SWidget> Keys();
	TSharedRef<SWidget> LastGame();
	TSharedRef<SWidget> EndCard();
	bool IsChosen(FUghMenu::ERow Row) const;

	TSharedPtr<const FUghUiState> State;
};
