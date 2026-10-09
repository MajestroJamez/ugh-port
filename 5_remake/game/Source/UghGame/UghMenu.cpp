#include "UghMenu.h"

#include "UghControls.h"
#include "UghPasswords.h"
#include "UghProfile.h"
#include "UghReplays.h"

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
	Screen = EScreen::Title;
	Highlight.Reset();
	NewRank = Profile.Scores.RankOf(End.Choice.Players, End.Score);
	NameEntry.Reset();
	if (NewRank != INDEX_NONE)
	{
		NameEntry.Emplace(Profile.Scores.LastName);
	}
}

FUghMenu::EAction FUghMenu::HandleKey(const FKey& Key)
{
	if (bShowingEnd)
	{
		return HandleEndKey(Key);
	}
	switch (Screen)
	{
	case EScreen::Settings:
		switch (SettingsMenu.HandleKey(Key, Profile.Settings, Options))
		{
		case FUghSettingsMenu::EResult::Changed: return EAction::Save;
		case FUghSettingsMenu::EResult::Controls: ControlsMenu.Open(); Screen = EScreen::Controls; break;
		case FUghSettingsMenu::EResult::Back: Screen = EScreen::Title; break;
		default: break;
		}
		return EAction::None;
	case EScreen::Controls:
		switch (ControlsMenu.HandleKey(Key, Profile.Settings.Keys))
		{
		case FUghControlsMenu::EResult::Changed: return EAction::Save;
		case FUghControlsMenu::EResult::Back: Screen = EScreen::Settings; break;
		default: break;
		}
		return EAction::None;
	case EScreen::Scores:
		Screen = EScreen::Title;   // any key
		Highlight.Reset();
		return EAction::None;
	case EScreen::Isles:
		if (Key == EKeys::R)
		{
			return WatchBest();
		}
		Isles.HandleKey(Key);   // (it closes going back, AdvanceIsles)
		return EAction::None;
	case EScreen::Replays:
		return HandleReplaysKey(Key);
	default:
		return HandleTitleKey(Key);
	}
}

FUghMenu::EAction FUghMenu::HandleEndKey(const FKey& Key)
{
	if (NameEntry && !NameEntry->HandleKey(Key == FUghControls::BackKey() ? EKeys::BackSpace : Key))
	{
		return EAction::None;
	}
	bShowingEnd = false;   // a key only closes it
	if (!NameEntry)
	{
		return EAction::None;
	}
	const FUghGameEnd& End = *LastGame;
	const FString Name = NameEntry->GetCarved();
	const int32 Rank = Profile.Scores.Insert(End.Choice.Players,
		{ Name, End.Score, End.Level, End.Choice.Difficulty, FUghHighScores::Today() });
	Profile.Scores.LastName = Name;
	NameEntry.Reset();
	Highlight = MakeTuple(End.Choice.Players, Rank);
	Screen = EScreen::Scores;
	return EAction::Save;
}

FUghMenu::EAction FUghMenu::HandleReplaysKey(const FKey& Key)
{
	if (!Replays || !Clipboard)
	{
		Screen = EScreen::Title;   // (none: nothing to show)
		return EAction::None;
	}
	switch (ReplaysMenu.HandleKey(Key, *Replays, *Clipboard))
	{
	case FUghReplaysMenu::EResult::Back:
		Screen = EScreen::Title;
		return EAction::None;
	case FUghReplaysMenu::EResult::Watch:
		ToWatch = ReplaysMenu.GetChosen(*Replays);
		WatchedFrom = EScreen::Replays;
		return EAction::Watch;
	case FUghReplaysMenu::EResult::OpenFolder:
		return EAction::OpenFolder;
	default:
		return EAction::None;
	}
}

FUghMenu::EAction FUghMenu::WatchBest()
{
	const FUghReplays::FEntry* Best = Replays && Isles.GetStage() == FUghIsles::EStage::Choose
		? Replays->Best(Isles.GetPlayers(), Isles.GetCursor()) : nullptr;
	if (!Best)
	{
		return EAction::None;
	}
	ToWatch = Best->Replay;
	WatchedFrom = EScreen::Title;
	Isles.Close();
	Screen = EScreen::Title;
	return EAction::Watch;
}

void FUghMenu::ShowAfterWatching()
{
	Screen = WatchedFrom;
	ToWatch.Reset();
}

FUghMenu::EAction FUghMenu::HandleTitleKey(const FKey& Key)
{
	if (Key == EKeys::Escape || (Key == EKeys::Enter && Row == ERow::Quit))
	{
		return EAction::Quit;
	}
	if (Key == EKeys::Enter && Row == ERow::Settings)
	{
		SettingsMenu.Open();
		Screen = EScreen::Settings;
		return EAction::None;
	}
	if (Key == EKeys::Enter && Row == ERow::Scores)
	{
		Screen = EScreen::Scores;
		return EAction::None;
	}
	if (Key == EKeys::Enter && Row == ERow::Replays)
	{
		ReplaysMenu.Open();
		Screen = EScreen::Replays;
		return EAction::None;
	}
	if (Key == EKeys::Enter)
	{
		return !IsPasswordKnown() ? EAction::None : bIslesOn ? EAction::Isles : EAction::Play;
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
	const int32 Last = Profile.Scores.LastLevel(Players);
	switch (Row)
	{
	case ERow::Players: Players = 3 - Players; break;
	case ERow::Difficulty: Difficulty = FMath::Clamp(Difficulty + Direction, 0, DifficultyCount - 1); break;
	case ERow::Password:
		// the level the mode's last game got to (its first level needs none), or none
		Password = Direction > 0 && Last > 0 && Last < Passwords.LevelCount(Players) ? Passwords.Get(Players, Last)
			: FString();
		break;
	default: break;
	}
}

int32 FUghMenu::PasswordLevel() const
{
	return Passwords.Find(Players, Password);
}

FUghGameChoice FUghMenu::GetChoice() const
{
	if (Screen == EScreen::Isles)
	{
		return { Players, Difficulty, Isles.GetCursor() };
	}
	return { Players, Difficulty, Password.IsEmpty() ? 0 : FMath::Max(PasswordLevel(), 0) };
}

const TCHAR* FUghMenu::DifficultyName(int32 Difficulty)
{
	static const TCHAR* const Names[DifficultyCount] = { TEXT("Easy"), TEXT("Medium"), TEXT("Hard") };
	return Names[FMath::Clamp(Difficulty, 0, DifficultyCount - 1)];
}

void FUghMenu::OpenIsles(const FUghCameraPose& From, const FVector& Home, double SeaZ, bool bFlight)
{
	const int32 Typed = Password.IsEmpty() ? INDEX_NONE : PasswordLevel();
	Isles.Open(Players, Passwords.LevelCount(Players), Profile.Scores, Typed, From, Home, SeaZ, bFlight);
	Screen = EScreen::Isles;
}

TOptional<FUghGameChoice> FUghMenu::AdvanceIsles(double Seconds, const FUghCameraPose& Game, double SeaZ)
{
	Isles.Advance(Seconds, Game, SeaZ);
	if (Screen != EScreen::Isles)
	{
		return {};
	}
	if (Isles.IsArrived())
	{
		const FUghGameChoice Chosen{ Players, Difficulty, Isles.GetCursor() };
		Isles.Close();
		Screen = EScreen::Title;
		return Chosen;
	}
	if (!Isles.IsOpen())
	{
		Screen = EScreen::Title;   // gone back
	}
	return {};
}
