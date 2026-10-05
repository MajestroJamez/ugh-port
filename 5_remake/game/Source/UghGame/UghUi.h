// The screen of the game over the scene: the menu and the HUD.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiState;

/**
 * The screen over the scene (Slate, built in code; AUghHud puts it into the game's viewport, which scales it by the
 * screen's DPI): the title screen with the menu (SUghMenuScreen), the screen of a game (SUghPlayScreen) or why there
 * is no game. Its look is UghUiStyle's; it only shows FUghUiState, which AUghHud takes from the game every frame.
 */
class SUghScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedPtr<const FUghUiState> State;
};
