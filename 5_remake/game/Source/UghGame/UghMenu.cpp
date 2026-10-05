#include "UghMenu.h"

#include "UghPasswords.h"

namespace
{
	/** The keys of the digits 0 .. 9 above the letters. */
	const FKey (&Digits())[10]
	{
		static const FKey Keys[10] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
			EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		return Keys;
	}
}

TCHAR FUghMenu::CharOf(const FKey& Key)
{
	const FString Name = Key.GetFName().ToString();
	if (Name.Len() == 1 && Name[0] >= TEXT('A') && Name[0] <= TEXT('Z'))
	{
		return Name[0];
	}
	for (int32 I = 0; I < UE_ARRAY_COUNT(Digits()); ++I)
	{
		if (Digits()[I] == Key)
		{
			return TEXT('0') + I;
		}
	}
	return 0;
}

FKey FUghMenu::KeyOf(TCHAR Char)
{
	if (Char >= TEXT('0') && Char <= TEXT('9'))
	{
		return Digits()[Char - TEXT('0')];
	}
	return Char >= TEXT('A') && Char <= TEXT('Z') ? FKey(FName(FString::Chr(Char))) : FKey();
}

void FUghMenu::ShowEnd(const FUghGameEnd& End)
{
	LastGame = End;
	bShowingEnd = true;
}

FUghMenu::EAction FUghMenu::HandleKey(const FKey& Key)
{
	if (bShowingEnd)
	{
		bShowingEnd = false;   // the key only closes it
		return EAction::None;
	}
	if (Key == EKeys::Escape || (Key == EKeys::Enter && Row == ERow::Quit))
	{
		return EAction::Quit;
	}
	if (Key == EKeys::Enter)
	{
		return IsPasswordKnown() ? EAction::Play : EAction::None;
	}
	if (Key == EKeys::Up || Key == EKeys::Down)
	{
		const int32 Step = Key == EKeys::Down ? 1 : RowCount - 1;
		Row = static_cast<ERow>((static_cast<int32>(Row) + Step) % RowCount);
	}
	else if (Key == EKeys::Left || Key == EKeys::Right)
	{
		Change(Key == EKeys::Right ? 1 : -1);
	}
	else if (Key == EKeys::BackSpace)
	{
		Password.LeftChopInline(1);
	}
	else if (const TCHAR Typed = CharOf(Key); Typed && Password.Len() < MaxPasswordLength)
	{
		Password.AppendChar(Typed);
	}
	return EAction::None;
}

void FUghMenu::Change(int32 Direction)
{
	switch (Row)
	{
	case ERow::Players: Players = 3 - Players; break;
	case ERow::Difficulty: Difficulty = FMath::Clamp(Difficulty + Direction, 0, DifficultyCount - 1); break;
	default: break;
	}
}

int32 FUghMenu::PasswordLevel() const
{
	return Passwords.Find(Players, Password);
}

FUghGameChoice FUghMenu::GetChoice() const
{
	return { Players, Difficulty, Password.IsEmpty() ? 0 : FMath::Max(PasswordLevel(), 0) };
}

const TCHAR* FUghMenu::DifficultyName(int32 Difficulty)
{
	static const TCHAR* const Names[DifficultyCount] = { TEXT("Easy"), TEXT("Medium"), TEXT("Hard") };
	return Names[FMath::Clamp(Difficulty, 0, DifficultyCount - 1)];
}
