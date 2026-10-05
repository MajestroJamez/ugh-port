#include "UghUi.h"

#include "UghUiMenu.h"
#include "UghUiParts.h"
#include "UghUiPlay.h"
#include "UghUiState.h"
#include "UghUiStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void SUghScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	using EScreen = FUghUiState::EScreen;
	auto Showing = [this](EScreen Screen)
	{
		return TAttribute<EVisibility>::CreateLambda([this, Screen]
		{
			return UghUiParts::ShownIf(State->Screen == Screen);
		});
	};
	ChildSlot
	[
		SNew(SOverlay).Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()[ SNew(SUghMenuScreen).State(State).Visibility(Showing(EScreen::Menu)) ]
		+ SOverlay::Slot()[ SNew(SUghPlayScreen).State(State).Visibility(Showing(EScreen::Play)) ]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
		[
			SNew(SBox).MaxDesiredWidth(900).Visibility(Showing(EScreen::Problem))
			[
				SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(32, 22))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)[ UghUiParts::Label(TEXT("No game"), UghUiStyle::Ember) ]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Font(UghUiStyle::Text(19)).ColorAndOpacity(UghUiStyle::Bone).AutoWrapText(true)
							.Text_Lambda([this] { return FText::FromString(State->Problem); })
					]
				]
			]
		]
	];
}
