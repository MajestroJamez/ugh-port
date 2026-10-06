#include "UghHighScores.h"

#include "Algo/IndexOf.h"
#include "Dom/JsonObject.h"
#include "Misc/DateTime.h"

namespace
{
	const TCHAR* const ModeFields[FUghHighScores::Modes] = { TEXT("onePlayer"), TEXT("team") };
}

int32 FUghHighScores::RankOf(int32 Players, uint32 Score) const
{
	const TArray<FEntry>& Entries = Table(Players);
	if (Score == 0)
	{
		return INDEX_NONE;
	}
	// below the same score reached earlier
	const int32 Rank = Algo::IndexOfByPredicate(Entries, [Score](const FEntry& Entry) { return Entry.Score < Score; });
	return Rank != INDEX_NONE ? Rank : Entries.Num() < Size ? Entries.Num() : INDEX_NONE;
}

int32 FUghHighScores::Insert(int32 Players, const FEntry& Entry)
{
	const int32 Rank = RankOf(Players, Entry.Score);
	if (Rank == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	TArray<FEntry>& Entries = Tables[ModeOf(Players)];
	FEntry& Added = Entries.Insert_GetRef(Entry, Rank);
	Added.Name = Added.Name.TrimStartAndEnd().Left(MaxName);
	Entries.SetNum(FMath::Min(Entries.Num(), Size));
	return Rank;
}

FString FUghHighScores::Today()
{
	return FDateTime::Now().ToString(TEXT("%Y-%m-%d"));
}

TSharedRef<FJsonObject> FUghHighScores::ToJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();
	for (int32 Mode = 0; Mode < Modes; ++Mode)
	{
		TArray<TSharedPtr<FJsonValue>> Entries;
		for (const FEntry& Entry : Tables[Mode])
		{
			TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
			Fields->SetStringField(TEXT("name"), Entry.Name);
			Fields->SetNumberField(TEXT("score"), Entry.Score);
			Fields->SetNumberField(TEXT("level"), Entry.Level);
			Fields->SetNumberField(TEXT("difficulty"), Entry.Difficulty);
			Fields->SetStringField(TEXT("day"), Entry.Day);
			Entries.Add(MakeShared<FJsonValueObject>(Fields));
		}
		TSharedRef<FJsonObject> Fields = MakeShared<FJsonObject>();
		Fields->SetArrayField(TEXT("best"), Entries);
		Fields->SetNumberField(TEXT("lastLevel"), LastLevels[Mode]);
		Json->SetObjectField(ModeFields[Mode], Fields);
	}
	Json->SetStringField(TEXT("lastName"), LastName);
	return Json;
}

FUghHighScores FUghHighScores::FromJson(const FJsonObject& Json)
{
	FUghHighScores Scores;
	Json.TryGetStringField(TEXT("lastName"), Scores.LastName);
	Scores.LastName = Scores.LastName.Left(MaxName);
	for (int32 Mode = 0; Mode < Modes; ++Mode)
	{
		const TSharedPtr<FJsonObject>* Fields = nullptr;
		if (!Json.TryGetObjectField(ModeFields[Mode], Fields))
		{
			continue;
		}
		int32 LastLevel = -1;
		(*Fields)->TryGetNumberField(TEXT("lastLevel"), LastLevel);
		Scores.LastLevels[Mode] = FMath::Max(LastLevel, -1);
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!(*Fields)->TryGetArrayField(TEXT("best"), Entries))
		{
			continue;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Entries)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			FEntry Entry;
			int64 Score = 0;
			if (!Value->TryGetObject(Object) || !(*Object)->TryGetStringField(TEXT("name"), Entry.Name) ||
				!(*Object)->TryGetNumberField(TEXT("score"), Score) || Score <= 0 || Score > MAX_uint32)
			{
				continue;
			}
			Entry.Score = uint32(Score);
			(*Object)->TryGetNumberField(TEXT("level"), Entry.Level);
			(*Object)->TryGetNumberField(TEXT("difficulty"), Entry.Difficulty);
			(*Object)->TryGetStringField(TEXT("day"), Entry.Day);
			Entry.Level = FMath::Max(Entry.Level, 0);
			Entry.Difficulty = FMath::Clamp(Entry.Difficulty, 0, 2);
			if (!Entry.Name.TrimStartAndEnd().IsEmpty())
			{
				Scores.Insert(Mode + 1, Entry);
			}
		}
	}
	return Scores;
}

bool FUghHighScores::operator==(const FUghHighScores& Other) const
{
	for (int32 Mode = 0; Mode < Modes; ++Mode)
	{
		if (Tables[Mode] != Other.Tables[Mode] || LastLevels[Mode] != Other.LastLevels[Mode])
		{
			return false;
		}
	}
	return LastName == Other.LastName;
}
