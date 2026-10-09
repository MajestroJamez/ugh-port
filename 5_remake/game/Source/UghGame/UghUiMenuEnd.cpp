// The title screen's parts about games: the last game, the card of a game's end with a high score's name, the keys.
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
			})) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Gamepad"), FText::FromString(TEXT("d-pad, A, B"))) ];
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
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 18, 0, 0)[ NameEntry() ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 24, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).Text(FText::FromString(TEXT("Press any key")))
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.ColorAndOpacity_Lambda([this] { return UghUiStyle::Bone.CopyWithNewOpacity(UghUiParts::Pulse(State->Time)); })
				.Visibility_Lambda([this] { return UghUiParts::ShownIf(!State->NameEntry); })
		]
		// the replay of the last level played: F5 saves it
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 0)
		[
			SNew(SHorizontalBox).Visibility_Lambda([this] { return UghUiParts::ShownIf(!State->LevelEnded.IsEmpty()); })
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				UghUiStyle::KeyHint(TEXT("F5"), TAttribute<FText>::CreateLambda([this]
				{
					return FText::FromString(State->LevelEndedSaved.IsEmpty()
						? FString::Printf(TEXT("save the replay - %s"), *State->LevelEnded)
						: FString::Printf(TEXT("replay saved: %s"), *State->LevelEndedSaved));
				}))
			]
		];
}

TSharedRef<SWidget> SUghMenuScreen::NameEntry()
{
	TSharedRef<SHorizontalBox> Letters = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < FUghNameEntry::MaxLength; ++Index)
	{
		auto IsCursor = [this, Index] { return State->NameEntry && State->NameEntry->GetCursor() == Index; };
		Letters->AddSlot().AutoWidth().Padding(3, 0)
		[
			SNew(SBox).WidthOverride(46).HeightOverride(60)
			[
				SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center)
					.BorderImage_Lambda([IsCursor] { return UghUiStyle::Field(IsCursor()); })
				[
					SNew(STextBlock).Font(UghUiStyle::Display(34))
						.Text_Lambda([this, Index, IsCursor]
						{
							const FString Name = State->NameEntry ? State->NameEntry->GetName() : FString();
							const bool bEmpty = Index >= Name.Len() || Name[Index] == TEXT(' ');
							// the cursor on nothing: a blinking stroke
							return FText::FromString(!bEmpty ? Name.Mid(Index, 1)
								: IsCursor() && UghUiParts::Pulse(State->Time) > 0.775f ? TEXT("_") : TEXT(""));
						})
						.ColorAndOpacity_Lambda([IsCursor] { return IsCursor() ? UghUiStyle::Amber : UghUiStyle::Bone; })
				]
			]
		];
	}
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(30, 16, 30, 14))
		.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->NameEntry.IsSet()); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(UghUiStyle::Display(30)).ColorAndOpacity(UghUiStyle::Amber)
					.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
					.Text(FText::FromString(TEXT("A new high score!")))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 4, 0, 0)
			[
				SNew(STextBlock).Font(UghUiStyle::Text(19, true)).ColorAndOpacity(UghUiStyle::Muted).Text_Lambda([this]
				{
					const int32 Place = State->NewRank + 1;
					const TCHAR* Suffix = Place == 1 ? TEXT("st") : Place == 2 ? TEXT("nd") : Place == 3 ? TEXT("rd") : TEXT("th");
					const TCHAR* Mode = State->LastGame && State->LastGame->Choice.Players == 2 ? TEXT("team") : TEXT("one player");
					return FText::FromString(FString::Printf(TEXT("%d%s of the ten best of %s"), Place, Suffix, Mode));
				})
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 14, 0, 4)
		[
			UghUiParts::Label(TEXT("Carve your name"), UghUiStyle::Muted)
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 14)[ Letters ]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			UghUiParts::KeyLine({ { TEXT("A-Z 0-9"), TEXT("type") }, { TEXT("↑ ↓"), TEXT("letter") },
				{ TEXT("← →"), TEXT("move") }, { TEXT("Enter"), TEXT("carve it") } })
		]
	];
}
