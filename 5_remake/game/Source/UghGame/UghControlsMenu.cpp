#include "UghControlsMenu.h"

#include "UghControls.h"

using FPlace = FUghKeyBindings::FPlace;

void FUghControlsMenu::Open()
{
	Row = 0;
	Column = 0;
	bCapturing = false;
	Notice.Reset();
}

FPlace FUghControlsMenu::GetPlace() const
{
	return FPlace{ Column / FUghKeyBindings::Slots, FMath::Min(Row, FUghKeyBindings::Actions - 1),
		Column % FUghKeyBindings::Slots };
}

FString FUghControlsMenu::NameOf(const FPlace& Place)
{
	return FString::Printf(TEXT("Pilot %d · %s"), Place.Pilot + 1, FUghKeyBindings::ActionName(Place.Action));
}

FUghControlsMenu::EResult FUghControlsMenu::HandleKey(const FKey& Key, FUghKeyBindings& Bindings)
{
	if (bCapturing)
	{
		return Capture(Key, Bindings);
	}
	Notice.Reset();
	const bool bKeyRow = Row < FUghKeyBindings::Actions;
	if (Key == EKeys::Escape || Key == FUghControls::BackKey() || (Key == EKeys::Enter && Row == BackRow))
	{
		return EResult::Back;
	}
	if (Key == EKeys::Up || Key == EKeys::Down)
	{
		Row = (Row + (Key == EKeys::Down ? 1 : RowCount - 1)) % RowCount;
	}
	else if ((Key == EKeys::Left || Key == EKeys::Right) && bKeyRow)
	{
		Column = (Column + (Key == EKeys::Right ? 1 : Columns - 1)) % Columns;
	}
	else if (Key == EKeys::Enter && Row == DefaultsRow)
	{
		const bool bChanged = !(Bindings == FUghKeyBindings::Defaults());
		Bindings = FUghKeyBindings::Defaults();
		Notice = TEXT("The remake's keys again");
		return bChanged ? EResult::Changed : EResult::None;
	}
	else if (Key == EKeys::Enter && bKeyRow)
	{
		bCapturing = true;
	}
	else if ((Key == EKeys::BackSpace || Key == EKeys::Delete) && bKeyRow && Bindings.Get(GetPlace()).IsValid())
	{
		Bindings.Clear(GetPlace());
		Notice = NameOf(GetPlace()) + TEXT(": no key");
		return EResult::Changed;
	}
	return EResult::None;
}

FUghControlsMenu::EResult FUghControlsMenu::Capture(const FKey& Key, FUghKeyBindings& Bindings)
{
	if (Key == EKeys::Escape || Key == FUghControls::BackKey())
	{
		bCapturing = false;
		Notice = TEXT("Left as it was");
		return EResult::None;
	}
	if (Key.IsGamepadKey())
	{
		Notice = TEXT("A key of the keyboard, please (the gamepads fly by themselves)");
		return EResult::None;
	}
	const FPlace Place = GetPlace();
	const FKey Before = Bindings.Get(Place);
	FPlace Other;
	switch (Bindings.Bind(Place, Key, &Other))
	{
	case FUghKeyBindings::EBound::Reserved:
		Notice = FUghKeyBindings::NameOf(Key) + TEXT(" is the game's own key: another one, please");
		return EResult::None;
	case FUghKeyBindings::EBound::Swapped:
		Notice = FString::Printf(TEXT("%s moved here from %s, which has %s now"), *FUghKeyBindings::NameOf(Key),
			*NameOf(Other), Bindings.Get(Other).IsValid() ? *FUghKeyBindings::NameOf(Bindings.Get(Other)) : TEXT("no key"));
		break;
	default:
		Notice = FString::Printf(TEXT("%s: %s"), *NameOf(Place), *FUghKeyBindings::NameOf(Key));
		break;
	}
	bCapturing = false;
	return Before == Key ? EResult::None : EResult::Changed;
}
