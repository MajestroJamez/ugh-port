// The replays of the levels: kept, the best of each, shared.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/**
 * A replay of a level (the logic's ugh_replay, docs/replay-format.md): what it is, its file and its text to share. The
 * logic plays it again exactly (FUghSimulation::Watch).
 */
class FUghReplay
{
public:
	/** Takes `Taken` (the logic's, ugh_logic_get_replay); none for none. */
	static TSharedPtr<FUghReplay> Of(ugh_replay* Taken);
	/** From the bytes of a .ughr file or its text ("UGHR1:..."); none, and why, when it is not one to play. */
	static TSharedPtr<FUghReplay> Read(const TArray<uint8>& Bytes, FString& OutError);
	static TSharedPtr<FUghReplay> Read(const FString& Text, FString& OutError);

	const ugh_replay_info& GetInfo() const { return Info; }
	const ugh_replay* GetHandle() const { return Handle; }
	int32 Players() const { return Info.start.players; }
	/** From 0 in the order of the mode. */
	int32 Level() const { return Info.start.level; }
	bool IsDone() const { return Info.done != 0; }
	/** The time of its play (seconds of the original's frames). */
	double Seconds() const;
	/** The first pilot's name, empty none. */
	FString Name() const;
	/** When it was played (local time), "2026-10-09 15:30"; empty unknown. */
	FString Date() const;

	/** The frontend's label: the level's password, the pilots' name (the team's twice), the date (now). */
	void Label(const FString& Password, const FString& PilotName, const FDateTime& Utc = FDateTime::UtcNow());

	/** The bytes of its .ughr file, its line of text. */
	TArray<uint8> Bytes() const;
	FString Text() const;
	/** Better than `Than` (none: nothing) as the best of its level and mode (ugh_replay_better). */
	bool IsBetterThan(const FUghReplay* Than) const;

	/** "1:23.4": a time of the play. */
	static FString Clock(double Seconds);

private:
	explicit FUghReplay(ugh_replay* Taken);

public:
	~FUghReplay() { ugh_replay_destroy(Handle); }
	FUghReplay(const FUghReplay&) = delete;
	FUghReplay& operator=(const FUghReplay&) = delete;

private:
	void Refresh();

	ugh_replay* Handle;   // owned
	ugh_replay_info Info;
};

/** The clipboard the replays are copied to and pasted from (the system's; a test's own). */
class IUghClipboard
{
public:
	virtual ~IUghClipboard() = default;
	virtual void Copy(const FString& Text) = 0;
	virtual FString Paste() = 0;
	/** The system's clipboard. */
	static IUghClipboard& System();
};

/**
 * The replays kept in a folder (`Saved/Replays` next to the profile, `-UghReplays=<folder>`), each a .ughr file: the
 * best of each level and mode (`best-1p-07.ughr`: kept automatically when a level ends better - only a level done, more
 * points, at the same points less time), the ones saved after a level (`1p-07-20261009-153012.ughr`), the ones imported
 * from the clipboard (`imported-...`) or put into the folder (any name). A file that is not a replay to play is listed
 * with the reason; one made by another version of the logic or other data says so (it may play differently). The
 * autopilot's store only reads (never writes).
 */
class FUghReplays
{
public:
	enum class EKind : uint8 { Best, Saved, Imported };

	struct FEntry
	{
		FString File;   // the full path
		EKind Kind = EKind::Saved;
		TSharedPtr<FUghReplay> Replay;   // none: refused (Problem says why)
		FString Problem;
		int32 OtherLogic = 0;   // UGH_REPLAY_OTHER_VERSION, UGH_REPLAY_OTHER_DATA (bits)
	};

	/** `-UghReplays=<folder>`, else the folder Replays next to the profile `ProfilePath`. */
	static FString DefaultFolder(const FString& ProfilePath);
	/** The name of the best replay of a level of a mode. */
	static FString BestName(int32 Players, int32 Level);

	/** Reads every .ughr of `Folder` (made when written to); compared with `Logic`; `bWritable` false: only reads. */
	void Open(const FString& InFolder, const ugh_logic* InLogic, bool bInWritable);
	const FString& GetFolder() const { return Folder; }
	bool IsWritable() const { return bWritable; }

	/** Every replay: by mode, level, the best first, then by date; the refused ones last. */
	const TArray<FEntry>& GetEntries() const { return Entries; }
	/** The best replay of a level of a mode, none. */
	const FEntry* Best(int32 Players, int32 Level) const;

	/** A level ended in a game: kept (written) as the best of its level and mode when it is better. True when it is. */
	bool OfferBest(const TSharedPtr<FUghReplay>& Replay);
	/** A new file of `Replay` (saved after a level, imported): its name; empty and why when it cannot be written. */
	FString Save(const TSharedPtr<FUghReplay>& Replay, EKind Kind, FString& OutError);
	/**
	 * Imports a replay from text or a file's bytes (white space around it skipped): saved as imported, unless the same
	 * one is kept already. What happened (or why not) in `OutMessage`; the replay, none when refused.
	 */
	TSharedPtr<FUghReplay> Import(const FString& Text, FString& OutMessage);
	/** Deletes the file of entry `Index`; false and why. */
	bool Delete(int32 Index, FString& OutError);

private:
	FString Folder;
	const ugh_logic* Logic = nullptr;
	bool bWritable = false;
	TArray<FEntry> Entries;

	FEntry Load(const FString& File) const;
	bool Write(const FString& File, const FUghReplay& Replay, FString& OutError);
	void Sort();
	static EKind KindOf(const FString& File);
};
