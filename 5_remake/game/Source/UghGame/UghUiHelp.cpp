// The screen during a game: the help of the keys and a setting just changed.
#include "UghUiPlay.h"

#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** A notice shows this long, then fades out in NoticeOut (seconds). */
	constexpr double NoticeFor = 1.2, NoticeOut = 0.4;
	/** The volume's bar of a notice, slate units. */
	constexpr float VolumeBar = 140;

	FText Fixed(const TCHAR* Text)
	{
		return FText::FromString(Text);
	}
}

TSharedRef<SWidget> SUghPlayScreen::Help()
{
	auto Row = [] { return SNew(SHorizontalBox); };
	TSharedRef<SHorizontalBox> Pilots = Row(), Game = Row(), Frontend = Row();
	Pilots->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("← ↑ → ↓"), Fixed(TEXT("fly"))) ];
	Pilots->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Space"), Fixed(TEXT("or"))) ];
	Pilots->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Right Ctrl"), Fixed(TEXT("fire"))) ];
	Pilots->AddSlot().AutoWidth()
	[
		SNew(SHorizontalBox).Visibility_Lambda([this] { return UghUiParts::ShownIf(State->Players == 2); })
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("W A S D"), Fixed(TEXT("pilot 2 flies"))) ]
		+ SHorizontalBox::Slot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Left Ctrl"), Fixed(TEXT("and fires"))) ]
	];
	Game->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("P"), Fixed(TEXT("pause"))) ];
	Game->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("Esc"), Fixed(TEXT("give up"))) ];
	Game->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("F1"), Fixed(TEXT("this help"))) ];
	Game->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("PgUp PgDn"), TAttribute<FText>::CreateLambda([this]
		{
			return FText::FromString(FString::Printf(TEXT("volume %d %%"), State->Volume));
		})) ];
	Frontend->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("U"), TAttribute<FText>::CreateLambda([this]
		{
			return FText::FromString(TEXT("upscaler: ") + State->Upscaler);
		})) ];
	Frontend->AddSlot().AutoWidth()[ UghUiStyle::KeyHint(TEXT("G"), Fixed(TEXT("frame generation"))) ];
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(26, 14, 8, 14))
		.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->Help > 0); })
		.ColorAndOpacity_Lambda([this] { return FLinearColor(1, 1, 1, State->Help); })
		.BorderBackgroundColor_Lambda([this] { return FLinearColor(1, 1, 1, State->Help); })
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[ UghUiParts::Label(TEXT("Controls"), UghUiStyle::Amber) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ Pilots ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ Game ]
		+ SVerticalBox::Slot().AutoHeight()[ Frontend ]
	];
}

TSharedRef<SWidget> SUghPlayScreen::Notice()
{
	auto Shown = [this]
	{
		return 1 - FMath::SmoothStep(0.f, 1.f, float((State->NoticeAge - NoticeFor) / NoticeOut));
	};
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 10))
		.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->NoticeAge < NoticeFor + NoticeOut); })
		.ColorAndOpacity_Lambda([Shown] { return FLinearColor(1, 1, 1, Shown()); })
		.BorderBackgroundColor_Lambda([Shown] { return FLinearColor(1, 1, 1, Shown()); })
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).ColorAndOpacity(UghUiStyle::Bone)
				.Text_Lambda([this] { return FText::FromString(State->Notice); })
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0, 0, 0)
		[
			// the volume as a bar
			SNew(SOverlay)
				.Visibility_Lambda([this] { return UghUiParts::ShownIf(State->NoticeLevel >= 0); })
			+ SOverlay::Slot()
			[
				SNew(SBox).WidthOverride(VolumeBar).HeightOverride(8)
				[
					SNew(SImage).Image(UghUiStyle::Rounded()).ColorAndOpacity(UghUiStyle::Bone.CopyWithNewOpacity(0.18f))
				]
			]
			+ SOverlay::Slot().HAlign(HAlign_Left)
			[
				SNew(SBox).HeightOverride(8).WidthOverride_Lambda([this] { return FOptionalSize(VolumeBar * State->NoticeLevel); })
				[
					SNew(SImage).Image(UghUiStyle::Rounded()).ColorAndOpacity(UghUiStyle::Amber)
				]
			]
		]
	];
}
