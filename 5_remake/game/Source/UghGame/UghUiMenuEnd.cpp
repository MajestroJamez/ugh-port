// The title screen's parts about games: the last game, the card of a game's end, the keys.
#include "UghUiMenu.h"

#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString ModeOf(const FUghGameChoice& Choice)
	{
		return FString::Printf(TEXT("%s, %s"), Choice.Players == 2 ? TEXT("Team") : TEXT("One player"),
			FUghMenu::DifficultyName(Choice.Difficulty));
	}

	/** A number under its label, for the card. */
	TSharedRef<SWidget> Figure(const FString& Label, const TAttribute<FText>& Text, const FSlateFontInfo& Font,
		const FLinearColor& Color)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[ UghUiParts::Label(Label, UghUiStyle::Muted) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 2, 0, 0)
			[
				SNew(STextBlock).Font(Font).ColorAndOpacity(Color).Text(Text)
					.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
			];
	}

	TSharedRef<SWidget> Divider()
	{
		return SNew(SBox).WidthOverride(1.5f).HeightOverride(56).Padding(0)[ SNew(SImage).Image(UghUiStyle::Line()) ];
	}
}

TSharedRef<SWidget> SUghMenuScreen::Keys()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("↑ ↓"), FText::FromString(TEXT("choose"))) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("← →"), FText::FromString(TEXT("change"))) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("A-Z 0-9"), FText::FromString(TEXT("password"))) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Enter"), FText::FromString(TEXT("select"))) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Esc"), FText::FromString(TEXT("quit"))) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("PgUp PgDn"), TAttribute<FText>::CreateLambda([this]
			{
				return FText::FromString(FString::Printf(TEXT("volume %d %%"), State->Volume));
			})) ];
}

TSharedRef<SWidget> SUghMenuScreen::LastGame()
{
	return SNew(SBox).WidthOverride(600).Visibility_Lambda([this]
		{
			return UghUiParts::ShownIf(State->LastGame && !State->bShowingEnd);
		})
	[
		SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 12))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18, 0)
			[
				UghUiParts::Label(TEXT("Last game"), UghUiStyle::Muted)
			]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(UghUiStyle::Text(17, true)).ColorAndOpacity(UghUiStyle::Bone).Text_Lambda([this]
				{
					const FUghGameEnd& End = State->LastGame.Get(FUghGameEnd());
					return FText::FromString(End.bAllDone ? FString(TEXT("All levels done!"))
						: FString::Printf(TEXT("Game over in level %d"), End.Level + 1));
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(UghUiStyle::Display(22)).ColorAndOpacity(UghUiStyle::Amber).Text_Lambda([this]
				{
					return UghUiParts::Score(State->LastGame.Get(FUghGameEnd()).Score);
				})
			]
		]
	];
}

TSharedRef<SWidget> SUghMenuScreen::EndCard()
{
	auto End = [this] { return State->LastGame.Get(FUghGameEnd()); };
	return SNew(SVerticalBox).Visibility_Lambda([this]
		{
			return UghUiParts::ShownIf(State->bShowingEnd);
		})
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()[ SNew(SImage).Image(&State->Pictures->Tablet) ]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 0, 0, 20)
			[
				UghUiParts::Carved(
					TAttribute<FText>::CreateLambda([End]
					{
						return FText::FromString(End().bAllDone ? TEXT("ALL LEVELS DONE!") : TEXT("GAME OVER"));
					}),
					TAttribute<FSlateFontInfo>::CreateLambda([End] { return UghUiStyle::Display(End().bAllDone ? 52 : 74); }))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 0)
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(34, 14))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Figure(TEXT("Level"), TAttribute<FText>::CreateLambda([End] { return FText::AsNumber(End().Level + 1); }),
						UghUiStyle::Display(34), UghUiStyle::Bone)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(30, 0)[ Divider() ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Figure(TEXT("Score"), TAttribute<FText>::CreateLambda([End] { return UghUiParts::Score(End().Score); }),
						UghUiStyle::Display(34), UghUiStyle::Amber)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(30, 0)[ Divider() ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					Figure(TEXT("Game"), TAttribute<FText>::CreateLambda([End] { return FText::FromString(ModeOf(End().Choice)); }),
						UghUiStyle::Text(22, true), UghUiStyle::Bone)
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 24, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).Text(FText::FromString(TEXT("Press any key")))
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.ColorAndOpacity_Lambda([this] { return UghUiStyle::Bone.CopyWithNewOpacity(UghUiParts::Pulse(State->Time)); })
		];
}
