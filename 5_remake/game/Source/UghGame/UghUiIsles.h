// The screen of the level selection.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

struct FUghUiState;

/**
 * The level selection over the archipelago (FUghIsles, UghUi): every stone's number on it (SUghIsleNumbers) in the
 * colour of its progress, the cursor's lit; what the cursor's level is (its number, done / open / locked, its
 * password), how many are done, why Enter did nothing; the keys in a line. Only shows FUghUiState.
 */
class SUghIslesScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUghIslesScreen) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

private:
	TSharedPtr<const FUghUiState> State;
};

/**
 * The stones' numbers (FUghUiState::Isles): a small tablet over each stone in the colour of its progress (green done,
 * amber open, red locked), smaller the further it is, the cursor's bigger with a bright rim, breathing.
 */
class SUghIsleNumbers : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUghIsleNumbers) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args) { State = Args._State; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TSharedPtr<const FUghUiState> State;
};
