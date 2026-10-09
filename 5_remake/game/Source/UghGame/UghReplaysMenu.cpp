#include "UghReplaysMenu.h"

#include "Misc/Paths.h"
#include "UghControls.h"
#include "UghReplays.h"

void FUghReplaysMenu::Open()
{
	Cursor = 0;
	Notice.Reset();
	bDeleting = false;
}

void FUghReplaysMenu::Say(const FString& Text)
{
	Notice = Text;
	++NoticeCount;
}

TSharedPtr<FUghReplay> FUghReplaysMenu::GetChosen(const FUghReplays& Replays) const
{
	const TArray<FUghReplays::FEntry>& Entries = Replays.GetEntries();
	return Entries.IsValidIndex(Cursor) ? Entries[Cursor].Replay : nullptr;
}

FUghReplaysMenu::EResult FUghReplaysMenu::HandleKey(const FKey& Key, FUghReplays& Replays, IUghClipboard& Clipboard)
{
	const TArray<FUghReplays::FEntry>& Entries = Replays.GetEntries();
	Cursor = FMath::Clamp(Cursor, 0, FMath::Max(Entries.Num() - 1, 0));
	const bool bWasDeleting = bDeleting;
	bDeleting = false;
	if (Key == EKeys::Escape || Key == FUghControls::BackKey())
	{
		return EResult::Back;
	}
	if (Key == EKeys::Up || Key == EKeys::Down)
	{
		Cursor = FMath::Clamp(Cursor + (Key == EKeys::Down ? 1 : -1), 0, FMath::Max(Entries.Num() - 1, 0));
		return EResult::None;
	}
	if (Key == EKeys::V)
	{
		FString Message;
		const TSharedPtr<FUghReplay> Imported = Replays.Import(Clipboard.Paste(), Message);
		Say(Message);
		const int32 Found = !Imported ? INDEX_NONE : Replays.GetEntries().IndexOfByPredicate(
			[&Imported](const FUghReplays::FEntry& Entry) { return Entry.Replay && Entry.Replay->Bytes() == Imported->Bytes(); });
		Cursor = Found != INDEX_NONE ? Found : Cursor;
		return EResult::None;
	}
	if (Key == EKeys::O)
	{
		Say(FString::Printf(TEXT("The folder %s: put a .ughr file there"), *Replays.GetFolder()));
		return EResult::OpenFolder;
	}
	if (!Entries.IsValidIndex(Cursor))
	{
		if (Key == EKeys::Enter || Key == EKeys::C || Key == EKeys::Delete)
		{
			Say(TEXT("No replay yet: finish a level, or press V to import one from the clipboard"));
		}
		return EResult::None;
	}
	const FUghReplays::FEntry& Entry = Entries[Cursor];
	if (Key == EKeys::Delete)
	{
		if (!bWasDeleting)
		{
			bDeleting = true;
			Say(FString::Printf(TEXT("Press Delete again to delete %s"), *FPaths::GetCleanFilename(Entry.File)));
			return EResult::None;
		}
		FString Error;
		const FString Name = FPaths::GetCleanFilename(Entry.File);
		Say(Replays.Delete(Cursor, Error) ? FString::Printf(TEXT("Deleted %s"), *Name) : Error);
		Cursor = FMath::Clamp(Cursor, 0, FMath::Max(Replays.GetEntries().Num() - 1, 0));
		return EResult::None;
	}
	if (!Entry.Replay)
	{
		if (Key == EKeys::Enter || Key == EKeys::C)
		{
			Say(FString::Printf(TEXT("%s: %s"), *FPaths::GetCleanFilename(Entry.File), *Entry.Problem));
		}
		return EResult::None;
	}
	if (Key == EKeys::C)
	{
		const FString Text = Entry.Replay->Text();
		Clipboard.Copy(Text);
		Say(FString::Printf(TEXT("Copied to the clipboard: %d characters to share (V imports them)"), Text.Len()));
		return EResult::None;
	}
	if (Key == EKeys::Enter)
	{
		return EResult::Watch;
	}
	return EResult::None;
}
