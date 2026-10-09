#include "UghUiReplays.h"

#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** The list's width and its columns (slate units of a screen 1080 high). */
	constexpr float ListWidth = 820, ModeWidth = 76, LevelWidth = 104, KindWidth = 124, ScoreWidth = 110, TimeWidth = 100;
	constexpr float RowHeight = 38;
	/** A notice fades out after this long (seconds). */
	constexpr double NoticeSeconds = 8;

	TSharedRef<STextBlock> Cell(const FSlateFontInfo& Font, const TAttribute<FSlateColor>& Color, TFunction<FString()> Of)
	{
		return SNew(STextBlock).Font(Font).ColorAndOpacity(Color).Text_Lambda([Of] { return FText::FromString(Of()); });
	}
}

void SUghReplaysScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Slot = 0; Slot < RowsShown; ++Slot)
	{
		Rows->AddSlot().AutoHeight()[ Row(Slot) ];
	}
	const TSharedRef<SWidget> Content = SNew(SBox).WidthOverride(ListWidth)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(12, 12))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ Rows ]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 10)
				[
					SNew(STextBlock).Font(UghUiStyle::Text(19)).ColorAndOpacity(UghUiStyle::Muted).AutoWrapText(true)
						.Text(FText::FromString(TEXT("No replays yet. The best of each level is kept when you finish it, "
							"F5 saves the last level's; V imports a friend's from the clipboard.")))
						.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->Replays.IsEmpty()); })
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)[ Details() ]
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 12, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).ColorAndOpacity(UghUiStyle::Amber).AutoWrapText(true)
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.Text_Lambda([this] { return FText::FromString(State->ReplaysNotice); })
				.Visibility_Lambda([this]
				{
					return UghUiParts::ShownIf(!State->ReplaysNotice.IsEmpty() && State->ReplaysNoticeAge < NoticeSeconds);
				})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 8, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(15)).ColorAndOpacity(UghUiStyle::Muted.CopyWithNewOpacity(0.75f))
				.Text_Lambda([this] { return FText::FromString(TEXT("Kept in ") + State->ReplaysFolder); })
		]
	];
	ChildSlot
	[
		UghUiParts::MenuPage(TEXT("Replays"), Content,
			UghUiParts::KeyLine({ { TEXT("↑ ↓"), TEXT("choose") }, { TEXT("Enter"), TEXT("watch") }, { TEXT("C"), TEXT("copy") },
				{ TEXT("V"), TEXT("paste") }, { TEXT("Del"), TEXT("delete") }, { TEXT("O"), TEXT("folder") },
				{ TEXT("Esc"), TEXT("back") } }))
	];
}

int32 SUghReplaysScreen::Top() const
{
	const int32 Count = State->Replays.Num();
	return FMath::Clamp(State->ReplaysCursor - RowsShown / 2 + 1, 0, FMath::Max(Count - RowsShown, 0));
}

const FUghUiReplay* SUghReplaysScreen::At(int32 Slot) const
{
	const int32 Index = Top() + Slot;
	return State->Replays.IsValidIndex(Index) ? &State->Replays[Index] : nullptr;
}

TSharedRef<SWidget> SUghReplaysScreen::Row(int32 Slot)
{
	auto IsChosen = [this, Slot] { return Top() + Slot == State->ReplaysCursor; };
	auto Color = [this, Slot, IsChosen](const FLinearColor& Normal)
	{
		return TAttribute<FSlateColor>::CreateLambda([this, Slot, IsChosen, Normal]
		{
			const FUghUiReplay* Replay = At(Slot);
			return Replay && !Replay->Problem.IsEmpty() ? UghUiStyle::Ember : IsChosen() ? UghUiStyle::Amber : Normal;
		});
	};
	auto Of = [this, Slot](TFunction<FString(const FUghUiReplay&)> Field)
	{
		return [this, Slot, Field] { const FUghUiReplay* Replay = At(Slot); return Replay ? Field(*Replay) : FString(); };
	};
	auto Valid = [this, Slot] { const FUghUiReplay* Replay = At(Slot); return Replay && Replay->Problem.IsEmpty(); };
	return SNew(SBox).HeightOverride(RowHeight).Visibility_Lambda([this, Slot] { return UghUiParts::ShownIf(At(Slot) != nullptr); })
	[
		SNew(SBorder).Padding(FMargin(12, 0)).VAlign(VAlign_Center)
			.BorderImage_Lambda([IsChosen] { return IsChosen() ? UghUiStyle::Chosen() : FStyleDefaults::GetNoBrush(); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(ModeWidth)[ Cell(UghUiStyle::Display(20), Color(UghUiStyle::Muted),
					Of([](const FUghUiReplay& R) { return R.Mode; })) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(LevelWidth)[ Cell(UghUiStyle::Text(20, true), Color(UghUiStyle::Bone),
					Of([](const FUghUiReplay& R) { return R.Problem.IsEmpty() ? FString::Printf(TEXT("Level %d"), R.Level + 1) : FString(); })) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(KindWidth).Visibility_Lambda([Valid] { return UghUiParts::ShownIf(Valid()); })
				[
					Cell(UghUiStyle::Text(17, true), TAttribute<FSlateColor>::CreateLambda([this, Slot]
						{
							const FUghUiReplay* Replay = At(Slot);
							return Replay && Replay->Kind == TEXT("best") ? UghUiStyle::Leaf : UghUiStyle::Muted;
						}),
						Of([](const FUghUiReplay& R) { return R.Kind.ToUpper(); }))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(ScoreWidth).HAlign(HAlign_Right).Visibility_Lambda([Valid] { return UghUiParts::ShownIf(Valid()); })
				[
					Cell(UghUiStyle::Display(22), UghUiStyle::Amber, Of([](const FUghUiReplay& R) { return R.Score; }))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(TimeWidth).HAlign(HAlign_Right).Visibility_Lambda([Valid] { return UghUiParts::ShownIf(Valid()); })
				[
					Cell(UghUiStyle::Text(19), UghUiStyle::Bone, Of([](const FUghUiReplay& R) { return R.Time; }))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(22, 0, 0, 0)
			[
				Cell(UghUiStyle::Text(19, true), Color(UghUiStyle::Bone),
					Of([](const FUghUiReplay& R) { return R.Problem.IsEmpty() ? (R.bDone ? R.Name : R.Name + TEXT(" (not done)")) : R.File + TEXT(": ") + R.Problem; }))
			]
		]
	];
}

TSharedRef<SWidget> SUghReplaysScreen::Details()
{
	auto Chosen = [this]() -> const FUghUiReplay*
	{
		return State->Replays.IsValidIndex(State->ReplaysCursor) ? &State->Replays[State->ReplaysCursor] : nullptr;
	};
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 12))
		.Visibility_Lambda([Chosen] { return UghUiParts::ShownIf(Chosen() && Chosen()->Problem.IsEmpty()); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Font(UghUiStyle::Text(20, true)).ColorAndOpacity(UghUiStyle::Bone).Text_Lambda([Chosen]
			{
				const FUghUiReplay* R = Chosen();
				return FText::FromString(!R ? FString() : FString::Printf(TEXT("%s, level %d, %s%s%s"),
					R->Mode == TEXT("Team") ? TEXT("Team") : TEXT("One player"), R->Level + 1, *R->Difficulty.ToLower(),
					R->Name.IsEmpty() ? TEXT("") : *(TEXT(", flown by ") + R->Name), R->Date.IsEmpty() ? TEXT("") : *(TEXT(", ") + R->Date)));
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(18)).ColorAndOpacity(UghUiStyle::Ember).AutoWrapText(true)
				.Text(FText::FromString(TEXT("Made by another version of the game or with other data: it may play differently.")))
				.Visibility_Lambda([Chosen] { return UghUiParts::ShownIf(Chosen() && Chosen()->bOtherLogic); })
		]
	];
}
