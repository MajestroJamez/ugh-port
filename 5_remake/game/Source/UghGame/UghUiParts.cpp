#include "UghUiParts.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** The groove of the gauge in its bone: from its left end, its top, its height (parts of the bone's height). */
	constexpr float GrooveEnds = 0.42f, GrooveTop = 0.37f, GrooveHeight = 0.26f;
	/** A score rises this much of the view's height in its time. */
	constexpr float PopupRise = 0.06f;
}

TSharedRef<SWidget> UghUiParts::Label(const FString& Text, const TAttribute<FSlateColor>& Color)
{
	FSlateFontInfo Font = UghUiStyle::Text(13.5f, true);
	Font.LetterSpacing = 260;
	return SNew(STextBlock).Font(Font).ColorAndOpacity(Color).Text(FText::FromString(Text.ToUpper()))
		.ShadowOffset(FVector2D(0, 1)).ShadowColorAndOpacity(UghUiStyle::Shadow);
}

TSharedRef<SWidget> UghUiParts::Carved(const TAttribute<FText>& Text, const TAttribute<FSlateFontInfo>& Font)
{
	return SNew(STextBlock).Font(Font).Text(Text).ColorAndOpacity(FLinearColor(0.035f, 0.028f, 0.02f, 0.92f))
		.ShadowOffset(FVector2D(0, 2.5f)).ShadowColorAndOpacity(FLinearColor(1.f, 0.93f, 0.8f, 0.4f));
}

float UghUiParts::Pulse(double Time)
{
	return 0.775f + 0.225f * FMath::Sin(float(Time) * 3.5f);
}

float UghUiParts::FadeIn(double Age, double In)
{
	return FMath::SmoothStep(0.f, 1.f, float(Age / FMath::Max(In, 1e-3)));
}

FText UghUiParts::Score(uint32 Score)
{
	return FText::AsNumber(Score);
}

EVisibility UghUiParts::ShownIf(bool bShown)
{
	return bShown ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

FVector2D SUghGauge::ComputeDesiredSize(float) const
{
	return State->Pictures ? State->Pictures->Bone.ImageSize : FVector2D(336, 42);
}

int32 SUghGauge::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const FLinearColor Tint = Style.GetColorAndOpacityTint();
	if (State->Pictures)
	{
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(), &State->Pictures->Bone,
			ESlateDrawEffect::None, Tint);
	}
	const FVector2f Size = FVector2f(Geometry.GetLocalSize());
	const FVector2f From(Size.Y * GrooveEnds, Size.Y * GrooveTop);
	const FVector2f Groove(Size.X - 2 * From.X, Size.Y * GrooveHeight);
	FSlateDrawElement::MakeBox(Elements, Layer + 1, Geometry.ToPaintGeometry(Groove, FSlateLayoutTransform(From)),
		UghUiStyle::Rounded(), ESlateDrawEffect::None, FLinearColor(0.03f, 0.02f, 0.015f, 0.85f) * Tint);
	const float Energy = FMath::Clamp(State->Energy, 0.f, 1.f);
	if (Energy > 0)
	{
		// green, amber, then red pulsing
		const FLinearColor Color = Energy > 0.5f ? UghUiStyle::Leaf : Energy > 0.25f ? UghUiStyle::Amber
			: UghUiStyle::Ember * FMath::Lerp(0.6f, 1.2f, UghUiParts::Pulse(State->Time * 2));
		const FVector2f Fill(FMath::Max(Groove.X * Energy, Groove.Y), Groove.Y);
		FSlateDrawElement::MakeBox(Elements, Layer + 2, Geometry.ToPaintGeometry(Fill, FSlateLayoutTransform(From)),
			UghUiStyle::Rounded(), ESlateDrawEffect::None, Color.CopyWithNewOpacity(1) * Tint);
		// a gloss along its top
		const FVector2f Gloss(Fill.X - 4, Fill.Y * 0.38f);
		FSlateDrawElement::MakeBox(Elements, Layer + 3,
			Geometry.ToPaintGeometry(Gloss, FSlateLayoutTransform(From + FVector2f(2, 1.5f))), UghUiStyle::Rounded(),
			ESlateDrawEffect::None, FLinearColor(1, 1, 1, 0.28f) * Tint);
	}
	return Layer + 3;
}

int32 SUghPopups::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	static const FSlateFontInfo Font = UghUiStyle::Display(28, 2);
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2f View = FVector2f(Geometry.GetLocalSize());
	for (const FUghUiPopup& Popup : State->Popups)
	{
		const FString Text = FString::Printf(TEXT("+%d"), Popup.Points);
		const FVector2f TextSize = FVector2f(Measure->Measure(Text, Font));
		// it pops up a little too big, then rises slower and slower and fades
		const float Age = FMath::Clamp(Popup.Age, 0.f, 1.f);
		const float Scale = Age < 0.08f ? FMath::Lerp(0.5f, 1.2f, Age / 0.08f)
			: FMath::Lerp(1.2f, 1.f, FMath::Min((Age - 0.08f) / 0.1f, 1.f));
		const FVector2f At = FVector2f(Popup.Where) * View - FVector2f(0, PopupRise * View.Y * (1 - FMath::Square(1 - Age)));
		const FVector2f Corner = At - FVector2f(TextSize.X / 2, TextSize.Y) * Scale;
		FSlateDrawElement::MakeText(Elements, Layer, Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(Scale, Corner)),
			Text, Font, ESlateDrawEffect::None, UghUiStyle::Amber.CopyWithNewOpacity(1 - Age * Age * Age));
	}
	return Layer;
}
