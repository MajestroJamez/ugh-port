#include "UghUiScores.h"

#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** A table's width, its columns (slate units of a screen 1080 high). */
	constexpr float TableWidth = 660, PlaceWidth = 46, LevelWidth = 76, PipsWidth = 64, ScoreWidth = 150;
	constexpr float RowHeight = 40;

	/** A row of the columns: the place, the name, the level, the difficulty, the score. */
	TSharedRef<SHorizontalBox> Columns(const TSharedRef<SWidget>& Place, const TSharedRef<SWidget>& Name,
		const TSharedRef<SWidget>& Level, const TSharedRef<SWidget>& Pips, const TSharedRef<SWidget>& Score)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(SBox).WidthOverride(PlaceWidth)[ Place ] ]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(8, 0)[ Name ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(LevelWidth).HAlign(HAlign_Center)[ Level ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(SBox).WidthOverride(PipsWidth)[ Pips ] ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(ScoreWidth).HAlign(HAlign_Right)[ Score ]
			];
	}
}

void SUghScoresScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1)[ SNew(SSpacer) ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock).Font(UghUiStyle::Display(64, 2)).ColorAndOpacity(UghUiStyle::Bone)
				.ShadowOffset(FVector2D(0, 4)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.Text(FText::FromString(TEXT("High scores")))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 2, 0, 28)
		[
			SNew(SBox).WidthOverride(150).HeightOverride(3)
			[
				SNew(SImage).Image(UghUiStyle::Rounded()).ColorAndOpacity(UghUiStyle::Amber)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0, 0, 20, 0)[ Table(1) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(20, 0, 0, 0)[ Table(2) ]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 28, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).Text(FText::FromString(TEXT("Press any key")))
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.ColorAndOpacity_Lambda([this] { return UghUiStyle::Bone.CopyWithNewOpacity(UghUiParts::Pulse(State->Time)); })
		]
		+ SVerticalBox::Slot().FillHeight(1)[ SNew(SSpacer) ]
	];
}

TSharedRef<SWidget> SUghScoresScreen::Table(int32 Players)
{
	const int32 Mode = Players == 2 ? 1 : 0;
	TSharedRef<SHorizontalBox> Copters = SNew(SHorizontalBox);
	for (int32 Copter = 0; Copter < Players; ++Copter)
	{
		Copters->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(SImage).Image(&State->Pictures->Copter)
		];
	}
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Place = 0; Place < FUghHighScores::Size; ++Place)
	{
		Rows->AddSlot().AutoHeight()[ Entry(Players, Place) ];
	}
	auto Muted = [](const TCHAR* Text) { return UghUiParts::Label(Text, UghUiStyle::Muted); };
	return SNew(SBox).WidthOverride(TableWidth)
	[
		SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(16, 16, 16, 14))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(10, 0, 0, 12)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)[ Copters ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(UghUiStyle::Display(32)).ColorAndOpacity(UghUiStyle::Bone)
						.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
						.Text(FText::FromString(Players == 2 ? TEXT("Team") : TEXT("One player")))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10, 0, 10, 6)
			[
				Columns(Muted(TEXT("#")), Muted(TEXT("Name")), Muted(TEXT("Level")), SNew(SBox), Muted(TEXT("Score")))
			]
			+ SVerticalBox::Slot().AutoHeight()[ Rows ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 10)
			[
				SNew(SBox).HeightOverride(1.5f)[ SNew(SImage).Image(UghUiStyle::Line()) ]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
				[
					Muted(TEXT("Last game got to"))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(UghUiStyle::Text(19, true)).ColorAndOpacity(UghUiStyle::Bone).Text_Lambda([this, Mode]
					{
						const int32 Level = State->LastLevels[Mode];
						return FText::FromString(Level < 0 ? FString(TEXT("no game yet"))
							: FString::Printf(TEXT("level %d"), Level + 1));
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(14, 0, 0, 0)
				[
					SNew(STextBlock).Font(UghUiStyle::Display(21)).ColorAndOpacity(UghUiStyle::Amber)
						.Text_Lambda([this, Mode] { return FText::FromString(State->LastPasswords[Mode]); })
				]
			]
		]
	];
}

TSharedRef<SWidget> SUghScoresScreen::Entry(int32 Players, int32 Place)
{
	auto Found = [this, Players, Place]() -> const FUghHighScores::FEntry*
	{
		const TArray<FUghHighScores::FEntry>& Table = State->Scores.Table(Players);
		return Table.IsValidIndex(Place) ? &Table[Place] : nullptr;
	};
	auto IsNew = [this, Players, Place]
	{
		return State->Highlight && State->Highlight->Key == Players && State->Highlight->Value == Place;
	};
	TSharedRef<SHorizontalBox> Pips = SNew(SHorizontalBox);
	for (int32 Pip = 0; Pip < 3; ++Pip)
	{
		Pips->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
		[
			SNew(SBox).WidthOverride(14).HeightOverride(6)
			[
				SNew(SImage).Image(UghUiStyle::Rounded()).ColorAndOpacity_Lambda([Found, Pip]
				{
					const FUghHighScores::FEntry* Entry = Found();
					return Entry && Pip <= Entry->Difficulty ? UghUiStyle::Amber.CopyWithNewOpacity(0.85f)
						: UghUiStyle::Bone.CopyWithNewOpacity(Entry ? 0.16f : 0.f);
				})
			]
		];
	}
	auto Text = [](const FSlateFontInfo& Font, const TAttribute<FSlateColor>& Color, TFunction<FString()> Of)
	{
		return SNew(STextBlock).Font(Font).ColorAndOpacity(Color).Text_Lambda([Of] { return FText::FromString(Of()); });
	};
	return SNew(SBox).HeightOverride(RowHeight)
	[
		SNew(SBorder).Padding(FMargin(10, 0)).VAlign(VAlign_Center)
			.BorderImage_Lambda([IsNew] { return IsNew() ? UghUiStyle::Chosen() : FStyleDefaults::GetNoBrush(); })
		[
			Columns(
				Text(UghUiStyle::Display(21), UghUiStyle::Muted, [Place] { return FString::Printf(TEXT("%d."), Place + 1); }),
				Text(UghUiStyle::Text(21, true), TAttribute<FSlateColor>::CreateLambda([Found, IsNew]
					{
						const FLinearColor Empty = UghUiStyle::Muted.CopyWithNewOpacity(0.4f);
						return IsNew() ? UghUiStyle::Amber : Found() ? UghUiStyle::Bone : Empty;
					}),
					[Found] { return Found() ? Found()->Name : FString(TEXT("—")); }),
				Text(UghUiStyle::Text(19), UghUiStyle::Muted,
					[Found] { return Found() ? FString::FromInt(Found()->Level + 1) : FString(); }),
				Pips,
				Text(UghUiStyle::Display(24), UghUiStyle::Amber,
					[Found] { return Found() ? UghUiParts::Score(Found()->Score).ToString() : FString(); }))
		]
	];
}
