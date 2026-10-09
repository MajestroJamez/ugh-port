#include "UghUiIsles.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** A number's tablet: how high (slate units, near), the cursor's bigger by this, its breath; the margin beside. */
	constexpr float TabletHeight = 40, CursorBigger = 1.35f, Breath = 0.06f, Beside = 7, Rim = 2.5f;
	/** A notice shows this long (seconds). */
	constexpr double NoticeSeconds = 3.5;

	FLinearColor ColorOf(EUghIsle State)
	{
		static const FLinearColor Locked = FLinearColor::FromSRGBColor(FColor(150, 44, 28));
		return State == EUghIsle::Done ? UghUiStyle::Leaf : State == EUghIsle::Open ? UghUiStyle::Amber : Locked;
	}

	const TCHAR* NameOf(EUghIsle State)
	{
		return State == EUghIsle::Done ? TEXT("done") : State == EUghIsle::Open ? TEXT("open")
			: TEXT("locked - its password opens it");
	}
}

void SUghIslesScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	auto Faded = [this] { return FLinearColor(1, 1, 1, State->IslesShown); };
	// (gone when faded out: the panels' outlines would stay)
	auto Seen = [this] { return State->IslesShown > 0.01f ? EVisibility::HitTestInvisible : EVisibility::Collapsed; };
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[ SNew(SUghIsleNumbers).State(State) ]
		+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(UghUiParts::ColumnPadding)
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(26, 16)).ColorAndOpacity_Lambda(Faded)
				.BorderBackgroundColor_Lambda(Faded).Visibility_Lambda(Seen)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ UghUiParts::Label(TEXT("Choose a level"), UghUiStyle::Amber) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
				[
					SNew(STextBlock).Font(UghUiStyle::Display(40, 2)).ColorAndOpacity(UghUiStyle::Bone)
						.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
						.Text_Lambda([this]
						{
							return FText::FromString(State->Choice.Players == 2 ? TEXT("Team") : TEXT("One player"));
						})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
				[
					SNew(STextBlock).Font(UghUiStyle::Text(18, true)).ColorAndOpacity(UghUiStyle::Bone)
						.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
						.Text_Lambda([this]
						{
							return FText::FromString(FString::Printf(TEXT("%d of %d levels done"), State->IslesDone,
								State->IslesCount));
						})
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 40)
		[
			SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).ColorAndOpacity_Lambda(Faded).Visibility_Lambda(Seen)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
				[
					SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(28, 12))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Font(UghUiStyle::Display(34)).ColorAndOpacity(UghUiStyle::Bone)
								.Text_Lambda([this]
								{
									return FText::FromString(FString::Printf(TEXT("Level %d"), State->IslesCursor + 1));
								})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(18, 4, 0, 0)
						[
							SNew(STextBlock).Font(UghUiStyle::Text(19, true))
								.ColorAndOpacity_Lambda([this] { return FSlateColor(ColorOf(State->IslesCursorState)); })
								.Text_Lambda([this] { return FText::FromString(NameOf(State->IslesCursorState)); })
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22, 4, 0, 0)
						[
							SNew(STextBlock).Font(UghUiStyle::Text(17)).ColorAndOpacity(UghUiStyle::Muted)
								.Text_Lambda([this]
								{
									// (a locked level's password is not given away)
									return FText::FromString(State->IslesCursorState == EUghIsle::Locked ? FString()
										: TEXT("password ") + State->IslesPassword);
								})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(22, 0, 0, 0)
						[
							// its best replay: R watches it
							SNew(SBox).Visibility_Lambda([this] { return UghUiParts::ShownIf(!State->IslesBest.IsEmpty()); })
							[
								UghUiStyle::KeyHint(TEXT("R"), TAttribute<FText>::CreateLambda([this]
								{
									return FText::FromString(TEXT("watch the best: ") + State->IslesBest);
								}))
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8, 0, 0)
				[
					SNew(STextBlock).Font(UghUiStyle::Text(18, true)).ColorAndOpacity(UghUiStyle::Ember)
						.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
						.Text_Lambda([this] { return FText::FromString(State->IslesNotice); })
						.Visibility_Lambda([this]
						{
							return State->IslesNoticeAge < NoticeSeconds && !State->IslesNotice.IsEmpty()
								? EVisibility::HitTestInvisible : EVisibility::Hidden;
						})
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
				[
					UghUiParts::KeyLine({ { TEXT("← → ↑ ↓"), TEXT("choose") },
						{ TEXT("Enter"), TEXT("fly there") }, { TEXT("Esc"), TEXT("back") },
						{ TEXT("any key"), TEXT("hurries a flight") } })
				]
			]
		]
	];
}

int32 SUghIsleNumbers::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const float Shown = State->IslesShown;
	if (Shown <= 0)
	{
		return Layer;
	}
	static const FSlateFontInfo Font = UghUiStyle::Display(26);
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2f View = FVector2f(Geometry.GetLocalSize());
	const float Breathing = 1 + Breath * FMath::Sin(float(State->Time) * 4.f);
	for (const FUghUiIsle& Isle : State->Isles)
	{
		const FString Text = FString::FromInt(Isle.Level + 1);
		const FVector2f TextSize = FVector2f(Measure->Measure(Text, Font));
		const float Scale = Isle.Size * (Isle.bCursor ? CursorBigger * Breathing : 1.f) * TabletHeight / TextSize.Y;
		const FVector2f Size(FMath::Max(TextSize.X * Scale + 2 * Beside * Isle.Size, TabletHeight * Isle.Size *
			(Isle.bCursor ? CursorBigger : 1.f)), TextSize.Y * Scale);
		// its bottom middle over the stone
		const FVector2f Corner = FVector2f(Isle.Where) * View - FVector2f(Size.X / 2, Size.Y);
		const FLinearColor Fill = ColorOf(Isle.State);
		const FLinearColor Edge = Isle.bCursor ? FLinearColor::White : FLinearColor(0, 0, 0, 0.7f);
		const float Around = Isle.bCursor ? Rim * 1.6f : Rim;
		FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(Size + FVector2f(2 * Around),
			FSlateLayoutTransform(Corner - FVector2f(Around))), UghUiStyle::Rounded(), ESlateDrawEffect::None,
			Edge.CopyWithNewOpacity(Edge.A * Shown));
		FSlateDrawElement::MakeBox(Elements, Layer + 1, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Corner)),
			UghUiStyle::Rounded(), ESlateDrawEffect::None, Fill.CopyWithNewOpacity(Shown));
		const FLinearColor Ink = Isle.State == EUghIsle::Locked ? UghUiStyle::Bone : UghUiStyle::Ink;
		const FVector2f TextCorner = Corner + (Size - TextSize * Scale) / 2;
		FSlateDrawElement::MakeText(Elements, Layer + 2, Geometry.ToPaintGeometry(TextSize,
			FSlateLayoutTransform(Scale, TextCorner)), Text, Font, ESlateDrawEffect::None, Ink.CopyWithNewOpacity(Shown));
		Layer += 3;
	}
	return Layer;
}
