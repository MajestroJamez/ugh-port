// The menu's screen of the replays.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FUghUiReplay;
struct FUghUiState;

/**
 * The menu's screen Replays in the title's column (FUghReplaysMenu): the replays kept (the best of each level and mode,
 * the saved ones, the imported ones; a refused file with its reason) as rows - the mode, the level, the kind, the points
 * earned, the time, the name -, the chosen one lit and told in full under them (the difficulty, the date, a replay of
 * another version of the logic), what the last key did, the folder, the keys. Only shows FUghUiState.
 */
class SUghReplaysScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghReplaysScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	/** Rows shown at once (the list scrolls with the cursor). */
	static constexpr int32 RowsShown = 10;

private:
	TSharedRef<SWidget> Row(int32 Slot);
	TSharedRef<SWidget> Details();
	/** The replay of row `Slot` of the window, none. */
	const FUghUiReplay* At(int32 Slot) const;
	int32 Top() const;

	TSharedPtr<const FUghUiState> State;
};
