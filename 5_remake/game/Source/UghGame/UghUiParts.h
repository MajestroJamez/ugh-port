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

	/** Where the title screen's column and the menu's screens in it stand (slate units of a screen 1080 high). */
	extern const FMargin ColumnPadding;
	/**
	 * A row of a setting: its name `LabelWidth` wide, its value between arrows to change it, shown while it is chosen
	 * (then the row is amber glass, its label amber).
	 */
	TSharedRef<SWidget> SettingRow(const FString& Name, float LabelWidth, const TFunction<bool()>& IsChosen,
		const TSharedRef<SWidget>& Value, const FMargin& Padding = FMargin(18, 11));
	/** A row that opens a screen, goes back or quits: its text, lit amber when chosen. */
	TSharedRef<SWidget> MenuItem(const FString& Text, const TFunction<bool()>& IsChosen);
	/** A screen of the menu in the title's column: its title big, what it shows, its keys at the bottom. */
	TSharedRef<SWidget> MenuPage(const FString& Title, const TSharedRef<SWidget>& Content, const TSharedRef<SWidget>& Keys);
	/** A bar of `Steps` stones lit amber up to `Level` (0 .. 1). */
	TSharedRef<SWidget> StepBar(const TAttribute<float>& Level, int32 Steps);
	/** A key cap and what it does. */
	struct FKeyHint
	{
		const TCHAR* Key;
		const TCHAR* What;
	};
	/** A line of key hints. */
	TSharedRef<SWidget> KeyLine(std::initializer_list<FKeyHint> Hints);
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

/**
 * The warnings over the copters flying fast enough to crash (FUghUiState::Warnings, UghWarning): !, !! or !!! over each,
 * amber to red the louder, blinking.
 */
class SUghWarnings : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SUghWarnings) {}
		SLATE_ARGUMENT(TSharedPtr<const FUghUiState>, State)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args) { State = Args._State; }
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TSharedPtr<const FUghUiState> State;
};
