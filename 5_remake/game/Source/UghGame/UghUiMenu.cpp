#include "UghUiMenu.h"

#include "UghUiParts.h"
#include "UghUiScores.h"
#include "UghUiSettings.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

using ERow = FUghMenu::ERow;
using EScreen = FUghMenu::EScreen;

namespace
{
	/** The menu's column: how wide (slate units of a screen 1080 high), its labels. */
	constexpr float MenuWidth = 600, LabelWidth = 150;
	/** How long the caret of the password is shown and hidden, seconds. */
	constexpr double CaretBlink = 1.1;
}

void SUghMenuScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	auto Showing = [this](EScreen Screen)
	{
		return TAttribute<EVisibility>::CreateLambda([this, Screen] { return UghUiParts::ShownIf(IsShowing(Screen)); });
	};
	ChildSlot
	[
		SNew(SOverlay)
		// the scene darker behind the column, and all of it behind the card of a game's end and the high scores
		+ SOverlay::Slot().HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride(1250).Visibility_Lambda([this]
				{
					return UghUiParts::ShownIf(!State->bShowingEnd && State->MenuScreen != EScreen::Scores);
				})
			[
				SNew(SImage).Image(&State->Pictures->Fade)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SImage).Image(UghUiStyle::Dim()).Visibility_Lambda([this]
			{
				return UghUiParts::ShownIf(State->bShowingEnd || State->MenuScreen == EScreen::Scores);
			})
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).Padding(UghUiParts::ColumnPadding)
		[
			SNew(SBox).Visibility(Showing(EScreen::Title))[ Title() ]
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).Padding(UghUiParts::ColumnPadding)
		[
			SNew(SUghSettingsScreen).State(State).Visibility(Showing(EScreen::Settings))
		]
		+ SOverlay::Slot().HAlign(HAlign_Left).Padding(UghUiParts::ColumnPadding)
		[
			SNew(SUghControlsScreen).State(State).Visibility(Showing(EScreen::Controls))
		]
		+ SOverlay::Slot()[ SNew(SUghScoresScreen).State(State).Visibility(Showing(EScreen::Scores)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[ EndCard() ]
	];
}

TSharedRef<SWidget> SUghMenuScreen::Title()
{
	FSlateFontInfo Tagline = UghUiStyle::Text(16, true);
	Tagline.LetterSpacing = 420;
	const FSlateFontInfo Value = UghUiStyle::Text(21, true);
	TSharedRef<SHorizontalBox> Pips = SNew(SHorizontalBox);
	for (int32 Pip = 0; Pip < FUghMenu::DifficultyCount; ++Pip)
	{
		Pips->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 6, 0)
		[
			SNew(SBox).WidthOverride(22).HeightOverride(8)
			[
				SNew(SImage).Image(UghUiStyle::Rounded()).ColorAndOpacity_Lambda([this, Pip]
				{
					return Pip <= State->Choice.Difficulty ? UghUiStyle::Amber : UghUiStyle::Bone.CopyWithNewOpacity(0.18f);
				})
			]
		];
	}
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(-40, 0, 0, 0)[ SNew(SImage).Image(&State->Pictures->Logo) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(6, -18, 0, 26)
		[
			SNew(STextBlock).Font(Tagline).ColorAndOpacity(UghUiStyle::Bone.CopyWithNewOpacity(0.85f))
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.Text(FText::FromString(TEXT("THE STONE AGE COPTER TAXI")))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left)
		[
			SNew(SBox).WidthOverride(MenuWidth)
			[
				SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(12)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()[ Setting(ERow::Players, TEXT("Players"), SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Font(Value).ColorAndOpacity(UghUiStyle::Bone).Text_Lambda([this]
							{
								return FText::FromString(State->Choice.Players == 2 ? TEXT("Team, two copters") : TEXT("One player"));
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 2, 0, 0)
						[
							SNew(STextBlock).Font(UghUiStyle::Text(16)).ColorAndOpacity(UghUiStyle::Muted).Text_Lambda([this]
							{
								return FText::FromString(FString::Printf(TEXT("%d levels"), State->Levels));
							})
						]) ]
					+ SVerticalBox::Slot().AutoHeight()[ Setting(ERow::Difficulty, TEXT("Difficulty"), SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 16, 0)
						[
							SNew(STextBlock).Font(Value).ColorAndOpacity(UghUiStyle::Bone).Text_Lambda([this]
							{
								return FText::FromString(FUghMenu::DifficultyName(State->Choice.Difficulty));
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ Pips ]) ]
					+ SVerticalBox::Slot().AutoHeight()[ Password() ]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 6)[ Play() ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						UghUiParts::MenuItem(TEXT("Settings"), [this] { return IsChosen(ERow::Settings); })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						UghUiParts::MenuItem(TEXT("High scores"), [this] { return IsChosen(ERow::Scores); })
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						UghUiParts::MenuItem(TEXT("Quit"), [this] { return IsChosen(ERow::Quit); })
					]
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 16, 0, 0)[ LastGame() ]
		+ SVerticalBox::Slot().FillHeight(1)[ SNew(SSpacer) ]
		+ SVerticalBox::Slot().AutoHeight()[ Keys() ];
}

bool SUghMenuScreen::IsChosen(ERow Row) const
{
	return State->Row == Row;
}

bool SUghMenuScreen::IsShowing(EScreen Screen) const
{
	return !State->bShowingEnd && State->MenuScreen == Screen;
}

TSharedRef<SWidget> SUghMenuScreen::Setting(ERow Row, const FString& Label, const TSharedRef<SWidget>& Value)
{
	return UghUiParts::SettingRow(Label, LabelWidth, [this, Row] { return IsChosen(Row); }, Value);
}

TSharedRef<SWidget> SUghMenuScreen::Password()
{
	FSlateFontInfo Typed = UghUiStyle::Display(22);
	Typed.LetterSpacing = 160;
	// the level the mode's last game got to (its first needs no password)
	auto Last = [this] { return State->LastLevels[State->Choice.Players == 2 ? 1 : 0]; };
	return SNew(SBorder).Padding(FMargin(18, 8))
		.BorderImage_Lambda([this] { return IsChosen(ERow::Password) ? UghUiStyle::Chosen() : FStyleDefaults::GetNoBrush(); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(LabelWidth)
				[
					UghUiParts::Label(TEXT("Password"), TAttribute<FSlateColor>::CreateLambda([this]
					{
						return IsChosen(ERow::Password) ? UghUiStyle::Amber : UghUiStyle::Muted;
					}))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
			[
				SNew(SBorder).Padding(FMargin(14, 5))
					.BorderImage_Lambda([this] { return UghUiStyle::Field(IsChosen(ERow::Password)); })
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(Typed).ColorAndOpacity(UghUiStyle::Bone)
							.Text_Lambda([this] { return FText::FromString(State->Password); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0, 0, 0)
					[
						SNew(STextBlock).Font(UghUiStyle::Display(22)).ColorAndOpacity(UghUiStyle::Amber)
							.Text(FText::FromString(TEXT("|"))).Visibility_Lambda([this]
							{
								const bool bOn = FMath::Fmod(State->Time, CaretBlink) < CaretBlink * 0.55;
								return IsChosen(ERow::Password) && bOn ? EVisibility::Visible : EVisibility::Hidden;
							})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock).Font(UghUiStyle::Text(17)).ColorAndOpacity(UghUiStyle::Muted.CopyWithNewOpacity(0.7f))
							.Text_Lambda([Last]
							{
								return FText::FromString(Last() > 0 ? TEXT("type one, or →") : TEXT("type a level's password"));
							})
							.Visibility_Lambda([this] { return State->Password.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
					]
					+ SHorizontalBox::Slot().FillWidth(1)[ SNew(SSpacer) ]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
					[
						SNew(STextBlock).Font(UghUiStyle::Text(16, true))
							.ColorAndOpacity_Lambda([this] { return State->bPasswordKnown ? UghUiStyle::Leaf : UghUiStyle::Ember; })
							.Text_Lambda([this, Last]
							{
								return FText::FromString(State->Password.IsEmpty()
									? (Last() > 0 ? FString::Printf(TEXT("last: level %d"), Last() + 1) : FString())
									: !State->bPasswordKnown ? FString(TEXT("unknown"))
									: FString::Printf(TEXT("level %d"), State->Choice.FirstLevel + 1));
							})
					]
				]
			]
		];
}

TSharedRef<SWidget> SUghMenuScreen::Play()
{
	auto Ink = [this](float Opacity)
	{
		return TAttribute<FSlateColor>::CreateLambda([this, Opacity]
		{
			const FLinearColor Color = IsChosen(ERow::Play) ? UghUiStyle::Ink : UghUiStyle::Bone;
			return Color.CopyWithNewOpacity(State->bPasswordKnown ? Opacity : Opacity * 0.4f);
		});
	};
	return SNew(SBox).HeightOverride(64)
	[
		SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.BorderImage_Lambda([this] { return UghUiStyle::Button(IsChosen(ERow::Play)); })
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Font(UghUiStyle::Display(28)).ColorAndOpacity(Ink(1)).Text(FText::FromString(TEXT("PLAY")))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 3, 0, 0)
			[
				SNew(STextBlock).Font(UghUiStyle::Text(17, true)).ColorAndOpacity(Ink(0.7f)).Text_Lambda([this]
				{
					return FText::FromString(State->bPasswordKnown
						? FString::Printf(TEXT("from level %d"), State->Choice.FirstLevel + 1) : FString(TEXT("no such password")));
				})
			]
		]
	];
}
