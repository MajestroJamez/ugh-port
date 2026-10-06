// The menu's screen of the pilots' keys.
#include "UghUiSettings.h"

#include "UghControlsMenu.h"
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
	/** The table: its width, the column of the logic keys' names, a cell (slate units of a screen 1080 high). */
	constexpr float Width = 940, NameWidth = 200;
	const FVector2D CellSize(160, 46);
}

void SUghControlsScreen::Construct(const FArguments& Args)
{
	State = Args._State;
	TSharedRef<SHorizontalBox> Header = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()[ SNew(SBox).WidthOverride(NameWidth) ];
	for (int32 Pilot = 0; Pilot < FUghKeyBindings::Pilots; ++Pilot)
	{
		Header->AddSlot().AutoWidth().Padding(4, 0)
		[
			SNew(SBox).WidthOverride(CellSize.X * FUghKeyBindings::Slots + 16).HAlign(HAlign_Center)
			[
				UghUiParts::Label(FString::Printf(TEXT("Pilot %d"), Pilot + 1), UghUiStyle::Amber)
			]
		];
	}
	TSharedRef<SVerticalBox> Table = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(18, 6, 0, 10)[ Header ];
	for (int32 Action = 0; Action < FUghKeyBindings::Actions; ++Action)
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(NameWidth)
				[
					SNew(STextBlock).Font(UghUiStyle::Text(21, true)).Text(FText::FromString(FUghKeyBindings::ActionName(Action)))
						.ColorAndOpacity_Lambda([this, Action]
						{
							return State->ControlsRow == Action ? UghUiStyle::Amber : UghUiStyle::Bone;
						})
				]
			];
		for (int32 Column = 0; Column < FUghControlsMenu::Columns; ++Column)
		{
			// a gap between the pilots
			const float Left = Column % FUghKeyBindings::Slots == 0 ? 8.f : 0.f;
			Row->AddSlot().AutoWidth().Padding(Left + 4, 4, 4, 4)[ Cell(Action, Column) ];
		}
		Table->AddSlot().AutoHeight().Padding(18, 0, 0, 0)[ Row ];
	}
	auto Chosen = [this](int32 Row) { return [this, Row] { return State->ControlsRow == Row; }; };
	Table->AddSlot().AutoHeight().Padding(0, 14, 0, 0)
	[
		UghUiParts::MenuItem(TEXT("Defaults: the remake's keys"), Chosen(FUghControlsMenu::DefaultsRow))
	];
	Table->AddSlot().AutoHeight()[ UghUiParts::MenuItem(TEXT("Back"), Chosen(FUghControlsMenu::BackRow)) ];

	const TSharedRef<SWidget> Content = SNew(SBox).WidthOverride(Width)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(12)[ Table ] ]
		+ SVerticalBox::Slot().AutoHeight().Padding(22, 12, 0, 0)
		[
			// what the last key did, or what the chosen cell waits for
			SNew(STextBlock).Font(UghUiStyle::Text(19, true)).ShadowOffset(FVector2D(0, 2))
				.ShadowColorAndOpacity(UghUiStyle::Shadow)
				.ColorAndOpacity_Lambda([this]
				{
					return State->bCapturing ? UghUiStyle::Amber.CopyWithNewOpacity(UghUiParts::Pulse(State->Time))
						: UghUiStyle::Amber;
				})
				.Text_Lambda([this]
				{
					const FUghKeyBindings::FPlace Place{ State->ControlsColumn / FUghKeyBindings::Slots,
						FMath::Min(State->ControlsRow, FUghKeyBindings::Actions - 1),
						State->ControlsColumn % FUghKeyBindings::Slots };
					return FText::FromString(State->bCapturing
						? FString::Printf(TEXT("Press a key for %s (Esc leaves it)"), *FUghControlsMenu::NameOf(Place))
						: State->ControlsNotice);
				})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 0)[ Gamepads() ]
	];
	ChildSlot
	[
		UghUiParts::MenuPage(TEXT("Controls"), Content, UghUiParts::KeyLine({ { TEXT("↑ ↓ ← →"), TEXT("choose") },
			{ TEXT("Enter"), TEXT("bind a key") }, { TEXT("Backspace"), TEXT("no key") }, { TEXT("Esc"), TEXT("back") } }))
	];
}

TSharedRef<SWidget> SUghControlsScreen::Cell(int32 Action, int32 Column)
{
	auto IsChosen = [this, Action, Column] { return State->ControlsRow == Action && State->ControlsColumn == Column; };
	const FUghKeyBindings::FPlace Place{ Column / FUghKeyBindings::Slots, Action, Column % FUghKeyBindings::Slots };
	// the arrows (a fallback font's: small, low) as big as the letters, raised
	auto IsArrow = [this, Place]
	{
		const FKey& Key = State->Settings.Keys.Get(Place);
		const FString Name = FUghKeyBindings::NameOf(Key);
		return Key.IsValid() && Name.Len() == 1 && !FChar::IsAlnum(Name[0]);
	};
	return SNew(SBox).WidthOverride(CellSize.X).HeightOverride(CellSize.Y)
	[
		SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center)
			.BorderImage_Lambda([this, IsChosen]
			{
				return IsChosen() && State->bCapturing ? UghUiStyle::Chosen() : UghUiStyle::Field(IsChosen());
			})
		[
			SNew(STextBlock)
				.Font_Lambda([IsArrow] { return UghUiStyle::Text(IsArrow() ? 34 : 20, true); })
				.RenderTransform_Lambda([IsArrow] { return FSlateRenderTransform(FVector2f(0, IsArrow() ? -7 : 0)); })
				.Text_Lambda([this, IsChosen, Place]
				{
					return FText::FromString(IsChosen() && State->bCapturing ? FString(TEXT("…"))
						: FUghKeyBindings::NameOf(State->Settings.Keys.Get(Place)));
				})
				.ColorAndOpacity_Lambda([this, IsChosen, Place]
				{
					return IsChosen() ? UghUiStyle::Amber
						: State->Settings.Keys.Get(Place).IsValid() ? UghUiStyle::Bone
						: UghUiStyle::Muted.CopyWithNewOpacity(0.5f);
				})
		]
	];
}

TSharedRef<SWidget> SUghControlsScreen::Gamepads()
{
	auto Line = [](const TCHAR* Text)
	{
		return SNew(STextBlock).Font(UghUiStyle::Text(18)).ColorAndOpacity(UghUiStyle::Bone).AutoWrapText(true)
			.Text(FText::FromString(Text));
	};
	return SNew(SBorder).BorderImage(UghUiStyle::Panel()).Padding(FMargin(22, 12))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[ UghUiParts::Label(TEXT("Gamepads"), UghUiStyle::Amber) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			Line(TEXT("The first gamepad flies pilot 1, the second pilot 2: the left stick or the d-pad fly, A or the "
				"right trigger fires, Start pauses, Back gives the game up, Y shows the help."))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
		[
			Line(TEXT("In the menu the d-pad chooses and changes, A selects, B goes back, X deletes."))
		]
	];
}
