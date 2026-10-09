#include "UghReplays.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* const Extension = TEXT(".ughr");
	/** A replay file's size at most (its steps and keys are bounded far below). */
	constexpr int64 MaxFileSize = 16 * 1024 * 1024;

	const TCHAR* ModeName(int32 Players) { return Players == 2 ? TEXT("team") : TEXT("1p"); }

	/** The system's clipboard (FPlatformApplicationMisc). */
	class FSystemClipboard : public IUghClipboard
	{
	public:
		virtual void Copy(const FString& Text) override { FPlatformApplicationMisc::ClipboardCopy(*Text); }
		virtual FString Paste() override
		{
			FString Text;
			FPlatformApplicationMisc::ClipboardPaste(Text);
			return Text;
		}
	};
}

IUghClipboard& IUghClipboard::System()
{
	static FSystemClipboard Clipboard;
	return Clipboard;
}

// ------------------------------------------------------------------ a replay

FUghReplay::FUghReplay(ugh_replay* Taken)
	: Handle(Taken)
{
	Refresh();
}

void FUghReplay::Refresh()
{
	ugh_replay_get_info(Handle, &Info);
}

TSharedPtr<FUghReplay> FUghReplay::Of(ugh_replay* Taken)
{
	return Taken ? TSharedPtr<FUghReplay>(new FUghReplay(Taken)) : nullptr;
}

TSharedPtr<FUghReplay> FUghReplay::Read(const TArray<uint8>& Bytes, FString& OutError)
{
	char Error[256] = "";
	ugh_replay* Read = ugh_replay_read(Bytes.GetData(), Bytes.Num(), Error, sizeof Error);
	OutError = Read ? FString() : FString(UTF8_TO_TCHAR(Error));
	return Of(Read);
}

TSharedPtr<FUghReplay> FUghReplay::Read(const FString& Text, FString& OutError)
{
	const FTCHARToUTF8 Utf8(*Text);
	return Read(TArray<uint8>(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()), OutError);
}

double FUghReplay::Seconds() const
{
	return Info.play_steps / (int32(UGH_LOGIC_FRAMES_PER_1000_S) / 1000.0);
}

FString FUghReplay::Name() const
{
	return Info.name_count > 0 ? FString(UTF8_TO_TCHAR(Info.names[0])) : FString();
}

FString FUghReplay::Date() const
{
	if (Info.date <= 0)
	{
		return FString();
	}
	const FDateTime Local = FDateTime::FromUnixTimestamp(Info.date) + (FDateTime::Now() - FDateTime::UtcNow());
	return Local.ToString(TEXT("%Y-%m-%d %H:%M"));
}

void FUghReplay::Label(const FString& Password, const FString& PilotName, const FDateTime& Utc)
{
	const FTCHARToUTF8 Pass(*Password), Name(*PilotName);
	ugh_replay_set_label(Handle, Pass.Get(), PilotName.IsEmpty() ? nullptr : Name.Get(), nullptr,
		Utc.ToUnixTimestamp());
	Refresh();
}

TArray<uint8> FUghReplay::Bytes() const
{
	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(int32(ugh_replay_write(Handle, nullptr, 0)));
	ugh_replay_write(Handle, Bytes.GetData(), Bytes.Num());
	return Bytes;
}

FString FUghReplay::Text() const
{
	TArray<char> Text;
	Text.SetNumUninitialized(int32(ugh_replay_write_text(Handle, nullptr, 0)) + 1);
	ugh_replay_write_text(Handle, Text.GetData(), Text.Num());
	return FString(UTF8_TO_TCHAR(Text.GetData()));
}

bool FUghReplay::IsBetterThan(const FUghReplay* Than) const
{
	return ugh_replay_better(Handle, Than ? Than->Handle : nullptr) != 0;
}

FString FUghReplay::Clock(double Seconds)
{
	const int32 Tenths = FMath::Max(0, FMath::RoundToInt32(Seconds * 10));
	return FString::Printf(TEXT("%d:%02d.%d"), Tenths / 600, Tenths / 10 % 60, Tenths % 10);
}

// ------------------------------------------------------------------ the store

FString FUghReplays::DefaultFolder(const FString& ProfilePath)
{
	FString Folder;
	if (FParse::Value(FCommandLine::Get(), TEXT("-UghReplays="), Folder))
	{
		return FPaths::ConvertRelativePathToFull(Folder);
	}
	return FPaths::GetPath(FPaths::ConvertRelativePathToFull(ProfilePath)) / TEXT("Replays");
}

FString FUghReplays::BestName(int32 Players, int32 Level)
{
	return FString::Printf(TEXT("best-%s-%02d%s"), ModeName(Players), Level + 1, Extension);
}

void FUghReplays::Open(const FString& InFolder, const ugh_logic* InLogic, bool bInWritable)
{
	Folder = InFolder;
	Logic = InLogic;
	bWritable = bInWritable;
	Entries.Reset();
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Folder / (FString(TEXT("*")) + Extension)), true, false);
	for (const FString& File : Files)
	{
		Entries.Add(Load(Folder / File));
	}
	Sort();
	UE_LOG(LogTemp, Display, TEXT("UGH replays: %d in %s"), Entries.Num(), *Folder);
}

FUghReplays::EKind FUghReplays::KindOf(const FString& File)
{
	const FString Name = FPaths::GetCleanFilename(File);
	if (Name.StartsWith(TEXT("best-")))
	{
		return EKind::Best;
	}
	// what the game saves after a level: <mode>-<NN>-<date>
	const bool bSaved = (Name.StartsWith(TEXT("1p-")) || Name.StartsWith(TEXT("team-"))) && !Name.Contains(TEXT("imported"));
	return bSaved ? EKind::Saved : EKind::Imported;
}

FUghReplays::FEntry FUghReplays::Load(const FString& File) const
{
	FEntry Entry;
	Entry.File = File;
	Entry.Kind = KindOf(File);
	TArray<uint8> Bytes;
	if (IFileManager::Get().FileSize(*File) > MaxFileSize || !FFileHelper::LoadFileToArray(Bytes, *File))
	{
		Entry.Problem = TEXT("cannot be read");
		return Entry;
	}
	Entry.Replay = FUghReplay::Read(Bytes, Entry.Problem);
	if (Entry.Replay && Logic)
	{
		Entry.OtherLogic = ugh_replay_compare_logic(Entry.Replay->GetHandle(), Logic);
	}
	return Entry;
}

void FUghReplays::Sort()
{
	Entries.StableSort([](const FEntry& A, const FEntry& B)
	{
		if (!A.Replay || !B.Replay)
		{
			return A.Replay && !B.Replay ? true : !A.Replay && B.Replay ? false : A.File < B.File;
		}
		const ugh_replay_info& X = A.Replay->GetInfo();
		const ugh_replay_info& Y = B.Replay->GetInfo();
		if (X.start.players != Y.start.players) return X.start.players < Y.start.players;
		if (X.start.level != Y.start.level) return X.start.level < Y.start.level;
		if (A.Kind != B.Kind) return A.Kind < B.Kind;
		return X.date > Y.date;
	});
}

const FUghReplays::FEntry* FUghReplays::Best(int32 Players, int32 Level) const
{
	return Entries.FindByPredicate([Players, Level](const FEntry& Entry)
	{
		return Entry.Kind == EKind::Best && Entry.Replay && Entry.Replay->Players() == Players &&
			Entry.Replay->Level() == Level;
	});
}

bool FUghReplays::Write(const FString& File, const FUghReplay& Replay, FString& OutError)
{
	if (!bWritable)
	{
		OutError = TEXT("the replays are not written (the autopilot)");
		return false;
	}
	IFileManager::Get().MakeDirectory(*Folder, true);
	if (!FFileHelper::SaveArrayToFile(Replay.Bytes(), *File))
	{
		OutError = FString::Printf(TEXT("cannot write %s"), *File);
		return false;
	}
	return true;
}

bool FUghReplays::OfferBest(const TSharedPtr<FUghReplay>& Replay)
{
	const FEntry* Kept = Best(Replay->Players(), Replay->Level());
	if (!Replay->IsBetterThan(Kept ? Kept->Replay.Get() : nullptr))
	{
		return false;
	}
	FString Error;
	const FString File = Folder / BestName(Replay->Players(), Replay->Level());
	if (!Write(File, *Replay, Error))
	{
		UE_CLOG(bWritable, LogTemp, Error, TEXT("UGH replays: %s"), *Error);
		return false;
	}
	Entries.RemoveAll([&File](const FEntry& Entry) { return Entry.File == File; });
	Entries.Add({ File, EKind::Best, Replay, FString(), Logic ? ugh_replay_compare_logic(Replay->GetHandle(), Logic) : 0 });
	Sort();
	return true;
}

FString FUghReplays::Save(const TSharedPtr<FUghReplay>& Replay, EKind Kind, FString& OutError)
{
	const FDateTime Now = FDateTime::Now();
	const FString Stem = FString::Printf(TEXT("%s%s-%02d-%s"), Kind == EKind::Imported ? TEXT("imported-") : TEXT(""),
		ModeName(Replay->Players()), Replay->Level() + 1, *Now.ToString(TEXT("%Y%m%d-%H%M%S")));
	FString File = Folder / Stem + Extension;
	for (int32 Other = 2; IFileManager::Get().FileExists(*File); ++Other)
	{
		File = Folder / FString::Printf(TEXT("%s-%d%s"), *Stem, Other, Extension);
	}
	if (!Write(File, *Replay, OutError))
	{
		return FString();
	}
	Entries.Add({ File, Kind, Replay, FString(), Logic ? ugh_replay_compare_logic(Replay->GetHandle(), Logic) : 0 });
	Sort();
	return FPaths::GetCleanFilename(File);
}

TSharedPtr<FUghReplay> FUghReplays::Import(const FString& Text, FString& OutMessage)
{
	FString Error;
	TSharedPtr<FUghReplay> Replay = FUghReplay::Read(Text.TrimStartAndEnd(), Error);
	if (!Replay)
	{
		OutMessage = FString::Printf(TEXT("Not imported: %s"), Text.TrimStartAndEnd().IsEmpty()
			? TEXT("the clipboard has no text") : *Error);
		return nullptr;
	}
	const TArray<uint8> Bytes = Replay->Bytes();
	const FString What = FString::Printf(TEXT("%s, level %d"), Replay->Players() == 2 ? TEXT("team") : TEXT("one player"),
		Replay->Level() + 1);
	if (Entries.ContainsByPredicate([&Bytes](const FEntry& Entry) { return Entry.Replay && Entry.Replay->Bytes() == Bytes; }))
	{
		OutMessage = FString::Printf(TEXT("Already kept: %s"), *What);
		return Replay;
	}
	if (Save(Replay, EKind::Imported, Error).IsEmpty())
	{
		OutMessage = FString::Printf(TEXT("Not imported: %s"), *Error);
		return nullptr;
	}
	const int32 Other = Logic ? ugh_replay_compare_logic(Replay->GetHandle(), Logic) : 0;
	OutMessage = FString::Printf(TEXT("Imported: %s%s"), *What,
		Other ? TEXT(" - made by another version of the game: it may play differently") : TEXT(""));
	return Replay;
}

bool FUghReplays::Delete(int32 Index, FString& OutError)
{
	if (!Entries.IsValidIndex(Index))
	{
		return false;
	}
	if (!bWritable || !IFileManager::Get().Delete(*Entries[Index].File))
	{
		OutError = FString::Printf(TEXT("cannot delete %s"), *FPaths::GetCleanFilename(Entries[Index].File));
		return false;
	}
	Entries.RemoveAt(Index);
	return true;
}
