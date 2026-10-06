#include "UghNameEntry.h"

#include "UghMenu.h"

namespace
{
	/** The characters a gamepad turns through. */
	const TCHAR Arcade[] = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
	constexpr int32 ArcadeLength = UE_ARRAY_COUNT(Arcade) - 1;
}

FUghNameEntry::FUghNameEntry(const FString& InName)
	: Name(InName.Left(MaxLength).ToUpper())
	, Cursor(FMath::Min(Name.Len(), MaxLength - 1))
{
}

FString FUghNameEntry::GetCarved() const
{
	const FString Carved = Name.TrimStartAndEnd();
	return Carved.IsEmpty() ? FString(TEXT("UGH")) : Carved;
}

bool FUghNameEntry::HandleKey(const FKey& Key)
{
	if (Key == EKeys::Enter || Key == EKeys::Escape)
	{
		return true;
	}
	if (Key == EKeys::Up || Key == EKeys::Down)
	{
		Turn(Key == EKeys::Up ? 1 : -1);
	}
	else if (Key == EKeys::Left)
	{
		Cursor = FMath::Max(Cursor - 1, 0);
	}
	else if (Key == EKeys::Right)
	{
		Cursor = FMath::Min3(Cursor + 1, Name.Len(), MaxLength - 1);
	}
	else if (Key == EKeys::BackSpace && Cursor < Name.Len())
	{
		Name.RemoveAt(Cursor);   // the letter at the cursor
	}
	else if (Key == EKeys::BackSpace && Cursor > 0)
	{
		Name.RemoveAt(--Cursor);   // after the name: its last letter
	}
	else if (Key == EKeys::SpaceBar)
	{
		Type(TEXT(' '));
	}
	else if (const TCHAR Typed = FUghMenu::CharOf(Key))
	{
		Type(Typed);
	}
	return false;
}

void FUghNameEntry::Turn(int32 Steps)
{
	if (Cursor == Name.Len())
	{
		Name.AppendChar(TEXT(' '));   // a new character, turned from the space: Up gives A
	}
	const TCHAR* Found = FCString::Strchr(Arcade, Name[Cursor]);
	const int32 Index = Found ? int32(Found - Arcade) : 0;
	Name[Cursor] = Arcade[(Index + Steps + ArcadeLength) % ArcadeLength];
}

void FUghNameEntry::Type(TCHAR Char)
{
	if (Cursor == Name.Len())
	{
		Name.AppendChar(Char);
	}
	else
	{
		Name[Cursor] = Char;
	}
	Cursor = FMath::Min(Cursor + 1, MaxLength - 1);
}
