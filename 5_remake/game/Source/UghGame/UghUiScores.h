// The menu's screen of the high scores.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiState;

/**
 * The menu's screen High scores (FUghHighScores) over the darkened scene: the ten best games of one player and of the
 * team side by side (the place, the name, the level, the difficulty as pips, the score; a new entry lit amber), under
 * each the level the mode's last game got to and its password; any key goes back. Only shows FUghUiState.
 */
class SUghScoresScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghScoresScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	/** The table of the mode of `Players` (1 or 2). */
	TSharedRef<SWidget> Table(int32 Players);
	TSharedRef<SWidget> Entry(int32 Players, int32 Place);

	TSharedPtr<const FUghUiState> State;
};
