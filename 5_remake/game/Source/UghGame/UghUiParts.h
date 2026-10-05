// Parts of the screen the menu and the play share.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

struct FUghUiState;
class SWidget;

/** Small pieces of UghUi: labels, carved text, a pulse. */
namespace UghUiParts
{
	/** A small label in spaced capitals (LEVEL, SCORE ...). */
	TSharedRef<SWidget> Label(const FString& Text, const TAttribute<FSlateColor>& Color);
	/** Text carved into the stone of a tablet: dark, a light lip below it. */
	TSharedRef<SWidget> Carved(const TAttribute<FText>& Text, const TAttribute<FSlateFontInfo>& Font);
	/** A slow pulse 0.55 .. 1 of the clock (a hint to press a key). */
	float Pulse(double Time);
	/** How much a thing shows `Age` seconds after it came up, fading in over `In` seconds. */
	float FadeIn(double Age, double In);
	/** A score as the screen writes it (thousands apart). */
	FText Score(uint32 Score);
	/** Shown (not taking the mouse) when `bShown`, else collapsed. */
	EVisibility ShownIf(bool bShown);
}

/** The energy gauge: a bone with a groove, filled green, amber, then red and pulsing as the energy runs low. */
class SUghGauge : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUghGauge) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args) { State = Args._State; }
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TSharedPtr<const FUghUiState> State;
};

/** The scores earned (FUghUiState::Popups): each pops up where it was earned, rises and fades. */
class SUghPopups : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUghPopups) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args) { State = Args._State; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TSharedPtr<const FUghUiState> State;
};
