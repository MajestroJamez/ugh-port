#include "UghUiSettings.h"

#include "UghUiParts.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

using ERow = FUghSettingsMenu::ERow;

namespace
{
	/** The screen's width and its labels' (slate units of a screen 1080 high). */
	constexpr float Width = 760, LabelWidth = 250;
	const TCHAR* const PartNames[] = { TEXT("Graphics"), TEXT("Sound"), TEXT("Game") };
	static_assert(UE_ARRAY_COUNT(PartNames) == UE_ARRAY_COUNT(FUghSettingsMenu::Parts));
}

void SUghSettingsScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
	for (int32 Index = 0; Index < FUghSettingsMenu::RowCount; ++Index)
	{
		const ERow Row = static_cast<ERow>(Index);
		for (int32 Part = 0; Part < UE_ARRAY_COUNT(PartNames); ++Part)
		{
			if (FUghSettingsMenu::Parts[Part] == Row)
			{
				Rows->AddSlot().AutoHeight().Padding(18, Part == 0 ? 4 : 12, 0, 4)
				[
					UghUiParts::Label(PartNames[Part], UghUiStyle::Amber)
				];
			}
		}
		if (Row == ERow::Controls || Row == ERow::Back)
		{
			Rows->AddSlot().AutoHeight().Padding(0, Row == ERow::Back ? 6 : 0, 0, 0)
			[
				UghUiParts::MenuItem(Row == ERow::Back ? TEXT("Back") : TEXT("Controls: the pilots' keys"),
					[this, Row] { return IsChosen(Row); })
			];
			continue;
		}
		Rows->AddSlot().AutoHeight()
		[
			UghUiParts::SettingRow(FUghSettingsMenu::LabelOf(Row), LabelWidth, [this, Row] { return IsChosen(Row); },
				Value(Row), FMargin(18, 6))
		];
	}

	const TSharedRef<SWidget> Content = SNew(SBox).WidthOverride(Width)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(12)[ Rows ]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 16, 0, 0)
		[
			SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 12))
			[
				SNew(STextBlock).Font(UghUiStyle::Text(18)).ColorAndOpacity(UghUiStyle::Bone).AutoWrapText(true)
					.Text_Lambda([this]
					{
						return FText::FromString(FUghSettingsMenu::HintOf(State->SettingsRow, State->Settings, State->Options));
					})
			]
		]
	];
	ChildSlot
	[
		UghUiParts::MenuPage(TEXT("Settings"), Content, UghUiParts::KeyLine({ { TEXT("↑ ↓"), TEXT("choose") },
			{ TEXT("← →"), TEXT("change") }, { TEXT("Enter"), TEXT("select") }, { TEXT("Esc"), TEXT("back") },
			{ TEXT("Gamepad"), TEXT("d-pad, A, B") } }))
	];
}

bool SUghSettingsScreen::IsChosen(ERow Row) const
{
	return State->SettingsRow == Row;
}

TSharedRef<SWidget> SUghSettingsScreen::Value(ERow Row)
{
	const bool bVolume = FUghSettingsMenu::LevelOf(Row, FUghSettings()) >= 0;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, bVolume ? 16 : 0, 0)
		[
			bVolume ? UghUiParts::StepBar(TAttribute<float>::CreateLambda([this, Row]
				{
					return FUghSettingsMenu::LevelOf(Row, State->Settings);
				}), 100 / FUghSettings::VolumeStep)
			: SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(STextBlock).Font(UghUiStyle::Text(21, true))
				.Text_Lambda([this, Row]
				{
					return FText::FromString(FUghSettingsMenu::ValueOf(Row, State->Settings, State->Options));
				})
				.ColorAndOpacity_Lambda([this, Row]
				{
					const bool bFixed = Row == ERow::FrameGeneration && State->Options.FrameGenerations.Num() < 2;
					return bFixed ? UghUiStyle::Muted : UghUiStyle::Bone;
				})
		];
}
