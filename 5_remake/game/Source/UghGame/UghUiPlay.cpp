#include "UghUiPlay.h"

#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "ugh_logic.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** The lives shown as copters; more are written beside them. */
	constexpr int32 LivesShown = 5;
	/** The status's distance from the screen's edges. */
	const FMargin Edges(28, 22);
	/** The caption: how long it takes to come up (seconds), how far it slides down meanwhile. */
	constexpr double CaptionIn = 0.6;
	constexpr float CaptionSlide = 26;
	/** Its tablet (slate units; smaller than the end of a game's). */
	const FVector2D CaptionSize(640, 210);
	/** The height of what a part of the status shows under its label. */
	constexpr float PartHeight = 44;

	TSharedRef<SWidget> Divider()
	{
		return SNew(SBox).WidthOverride(1.5f).HeightOverride(52)[ SNew(SImage).Image(UghUiStyle::Line()) ];
	}

	/** A part of the status: its label above it. */
	TSharedRef<SWidget> Part(const FString& Label, const TSharedRef<SWidget>& Content, EHorizontalAlignment Align = HAlign_Left)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(Align)[ UghUiParts::Label(Label, UghUiStyle::Muted) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(Align).Padding(0, 3, 0, 0)
			[
				SNew(SBox).HeightOverride(PartHeight).VAlign(VAlign_Center)[ Content ]
			];
	}
}

void SUghPlayScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[ SNew(SUghPopups).State(State) ]
		+ SOverlay::Slot()[ SNew(SUghWarnings).State(State) ]
		+ SOverlay::Slot()
		[
			// the status fades with the play (FUghUiState::Shown: none but in the play)
			SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush()).Padding(Edges)
				.ColorAndOpacity_Lambda([this] { return FLinearColor(1, 1, 1, State->Shown); })
				.Visibility_Lambda([this]
				{
					// (the panels' edges would show through a clear colour)
					return UghUiParts::ShownIf(State->Shown > 0);
				})
			[
				SNew(SOverlay)
				+ SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top)[ Status() ]
				+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top)[ Score() ]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 0, 0, 120)[ Caption() ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(0, 0, 0, 34)[ Help() ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, 28, 0, 0)[ Notice() ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0, 22, 0, 0)[ Watching() ]
	];
}

TSharedRef<SWidget> SUghPlayScreen::Status()
{
	TSharedRef<SHorizontalBox> Lives = SNew(SHorizontalBox);
	for (int32 Life = 0; Life < LivesShown; ++Life)
	{
		Lives->AddSlot().AutoWidth().Padding(0, 0, 4, 0)
		[
			SNew(SImage).Image(&State->Pictures->Copter).Visibility_Lambda([this, Life]
			{
				return UghUiParts::ShownIf(Life < State->Lives);
			})
		];
	}
	Lives->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0, 0, 0)
	[
		SNew(STextBlock).Font(UghUiStyle::Display(22)).ColorAndOpacity(UghUiStyle::Bone)
			.Text_Lambda([this] { return FText::FromString(FString::Printf(TEXT("+%d"), State->Lives - LivesShown)); })
			.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->Lives > LivesShown); })
	];
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 10, 24, 12))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top)
		[
			Part(TEXT("Level"), SNew(STextBlock).Font(UghUiStyle::Display(32)).ColorAndOpacity(UghUiStyle::Bone)
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.Text_Lambda([this] { return FText::AsNumber(State->Level + 1); }), HAlign_Center)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20, 0)[ Divider() ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Part(TEXT("Lives"), Lives)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(20, 0)[ Divider() ]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Part(TEXT("Energy"), SNew(SUghGauge).State(State))
		]
	];
}

TSharedRef<SWidget> SUghPlayScreen::Score()
{
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(24, 10, 18, 12))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			Part(TEXT("Score"), SNew(STextBlock).Font(UghUiStyle::Display(32)).ColorAndOpacity(UghUiStyle::Bone)
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.Text_Lambda([this] { return UghUiParts::Score(State->Score); }), HAlign_Right)
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 0, 0)
		[
			// the multiplier: lit when it multiplies
			SNew(SBox).MinDesiredWidth(58).HeightOverride(58)
			[
				SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center).BorderImage_Lambda([this]
					{
						return UghUiStyle::Button(State->Multiplier > 1);
					})
				[
					SNew(STextBlock).Font(UghUiStyle::Display(24))
						.Text_Lambda([this] { return FText::FromString(FString::Printf(TEXT("×%d"), State->Multiplier)); })
						.ColorAndOpacity_Lambda([this]
						{
							return State->Multiplier > 1 ? UghUiStyle::Ink : UghUiStyle::Bone.CopyWithNewOpacity(0.55f);
						})
				]
			]
		]
	];
}

TSharedRef<SWidget> SUghPlayScreen::Caption()
{
	auto Shown = [this] { return UghUiParts::FadeIn(State->CaptionAge, CaptionIn); };
	FSlateFontInfo Small = UghUiStyle::Text(20, true);
	Small.LetterSpacing = 650;
	return SNew(SBorder).BorderImage(FStyleDefaults::GetNoBrush())
		.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->Phase == UGH_LOGIC_PHASE_CAPTION); })
		.ColorAndOpacity_Lambda([Shown] { return FLinearColor(1, 1, 1, Shown()); })
		.RenderTransform_Lambda([Shown] { return FSlateRenderTransform(FVector2f(0, -CaptionSlide * (1 - Shown()))); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBox).WidthOverride(CaptionSize.X).HeightOverride(CaptionSize.Y)[ SNew(SImage).Image(&State->Pictures->Tablet) ]
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(0, 14, 0, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(14, 0, 0, 0)
				[
					UghUiParts::Carved(FText::FromString(TEXT("LEVEL")), Small)
				]
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, -24, 0, 0)
				[
					UghUiParts::Carved(TAttribute<FText>::CreateLambda([this] { return FText::AsNumber(State->Level + 1); }),
						UghUiStyle::Display(96))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 0)
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 8))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 14, 0)
				[
					UghUiParts::Label(TEXT("Password"), UghUiStyle::Muted)
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(UghUiStyle::Display(22)).ColorAndOpacity(UghUiStyle::Amber)
						.Text_Lambda([this] { return FText::FromString(State->LevelPassword); })
				]
			]
		]
		// the level just done: its points, its time, the best of the level; F5 saves its replay
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 10, 0, 0)
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 8))
				.Visibility_Lambda([this]
				{
					return UghUiParts::ShownIf(!State->bWatching && State->bLevelEndedDone &&
						State->Level == State->LevelEndedLevel + 1);
				})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock).Font(UghUiStyle::Text(19, true)).ColorAndOpacity(UghUiStyle::Bone)
						.Text_Lambda([this] { return FText::FromString(State->LevelEnded); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 0, 0, 0)
				[
					SNew(STextBlock).Font(UghUiStyle::Display(19)).ColorAndOpacity(UghUiStyle::Leaf)
						.Text(FText::FromString(TEXT("NEW BEST")))
						.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->bLevelEndedBest); })
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 0, 0)
				[
					UghUiStyle::KeyHint(TEXT("F5"), TAttribute<FText>::CreateLambda([this]
					{
						return FText::FromString(State->LevelEndedSaved.IsEmpty() ? TEXT("save its replay") : TEXT("replay saved"));
					}))
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 18, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).Text(FText::FromString(TEXT("Press any key")))
				.ShadowOffset(FVector2D(0, 2)).ShadowColorAndOpacity(UghUiStyle::Shadow)
				.ColorAndOpacity_Lambda([this] { return UghUiStyle::Bone.CopyWithNewOpacity(UghUiParts::Pulse(State->Time)); })
		]
	];
}

TSharedRef<SWidget> SUghPlayScreen::Watching()
{
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(16, 6))
		.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->bWatching); })
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Display(19)).ColorAndOpacity(UghUiStyle::Amber).Text(FText::FromString(TEXT("REPLAY")))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(16, true)).ColorAndOpacity(UghUiStyle::Bone)
				.Text_Lambda([this] { return FText::FromString(State->WatchTitle); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(12, 0, 0, 0)
		[
			SNew(STextBlock).Font(UghUiStyle::Display(16)).ColorAndOpacity(UghUiStyle::Muted)
				.Text_Lambda([this] { return FText::FromString(State->WatchClock); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0, 0, 0)
		[
			UghUiStyle::KeyHint(TEXT("Esc"), FText::FromString(TEXT("stop")))
		]
	];
}
