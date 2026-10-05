// The screen during a game.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiState;

/**
 * The screen during a game (UghUi): the status in two panels at the top (the level, the lives as bone copters, the
 * energy as a bone gauge; the score with its multiplier), fading with the play; the scores earned rising; the
 * caption of a level as a stone tablet over the flight to the stone (its number carved, its password, a hint to press
 * a key); the help of the keys (F1, and shown at the first level); a setting just changed (the volume, the
 * upscaler). Only shows FUghUiState.
 */
class SUghPlayScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghPlayScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedRef<SWidget> Status();
	TSharedRef<SWidget> Score();
	TSharedRef<SWidget> Caption();
	TSharedRef<SWidget> Help();
	TSharedRef<SWidget> Notice();

	TSharedPtr<const FUghUiState> State;
};
