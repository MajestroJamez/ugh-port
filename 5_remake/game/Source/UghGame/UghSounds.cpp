#include "UghSounds.h"

#include "Misc/FileHelper.h"
#include "ugh_logic.h"

namespace
{
	using EAction = FUghSounds::EAction;

	/** Where the port plays them: Objects.kt (the enemies), Passengers.kt, Level.kt (the caption). */
	const FUghSounds::FCue CueTable[] = {
		{ UGH_LOGIC_EVENT_LEVEL_CAPTION, TEXT("caption"), EAction::Play },
		{ UGH_LOGIC_EVENT_FLYER_SCREECH, TEXT("screech"), EAction::Play },
		{ UGH_LOGIC_EVENT_FLYER_FLAP_START, TEXT("flap"), EAction::Loop },
		{ UGH_LOGIC_EVENT_FLYER_FLAP_STOP, TEXT("flap"), EAction::Stop },
		{ UGH_LOGIC_EVENT_BLOWER_BLOW, TEXT("blow"), EAction::Play },
		{ UGH_LOGIC_EVENT_TREE_DROP, TEXT("bonus"), EAction::Play },
		{ UGH_LOGIC_EVENT_QUICK_DELIVERY, TEXT("bonus"), EAction::Play },
		{ UGH_LOGIC_EVENT_PASSENGER_DROPPED, TEXT("drop"), EAction::Play },
	};

	uint32 Read32(const TArray<uint8>& Bytes, int32 At)
	{
		return Bytes[At] | Bytes[At + 1] << 8 | Bytes[At + 2] << 16 | uint32(Bytes[At + 3]) << 24;
	}

	uint16 Read16(const TArray<uint8>& Bytes, int32 At) { return uint16(Bytes[At] | Bytes[At + 1] << 8); }

	bool IsTag(const TArray<uint8>& Bytes, int32 At, const char* Tag)
	{
		return FMemory::Memcmp(Bytes.GetData() + At, Tag, 4) == 0;
	}
}

TConstArrayView<FUghSounds::FCue> FUghSounds::Cues()
{
	return CueTable;
}

const FUghSounds::FCue* FUghSounds::CueOf(int32 Event)
{
	for (const FCue& Cue : CueTable)
	{
		if (Cue.Event == Event)
		{
			return &Cue;
		}
	}
	return nullptr;
}

TArray<const TCHAR*> FUghSounds::Names()
{
	TArray<const TCHAR*> Result = { MenuMusic, GameMusic, EndingMusic, GameOver };
	for (const FCue& Cue : CueTable)
	{
		if (!Result.ContainsByPredicate([&Cue](const TCHAR* Name) { return FCString::Strcmp(Name, Cue.Name) == 0; }))
		{
			Result.Add(Cue.Name);
		}
	}
	return Result;
}

void FUghSounds::Load(const FString& Dir)
{
	Samples.Reset();
	SampleRate = 0;
	TArray<FString> Missing;
	for (const TCHAR* Name : Names())
	{
		TArray<int16> Sound;
		FString Error;
		if (ReadWav(Dir / FString(Name) + TEXT(".wav"), SampleRate, Sound, SampleRate, Error))
		{
			Samples.Add(Name, MoveTemp(Sound));
		}
		else
		{
			Missing.Add(Error);
		}
	}
	if (!Missing.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UGH silent without (.\\gradlew.bat :extractor:sound): %s"),
			*FString::Join(Missing, TEXT("; ")));
	}
}

bool FUghSounds::ReadWav(const FString& Path, int32 ExpectedRate, TArray<int16>& OutSamples, int32& OutRate,
	FString& OutError)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent))
	{
		OutError = Path + TEXT(": cannot read");
		return false;
	}
	if (Bytes.Num() < 12 || !IsTag(Bytes, 0, "RIFF") || !IsTag(Bytes, 8, "WAVE"))
	{
		OutError = Path + TEXT(": not a WAV file");
		return false;
	}
	int32 Rate = 0;
	bool bFormat = false;
	// the chunks: "fmt " (PCM, one channel, 16 bits) before "data"
	for (int32 At = 12; At + 8 <= Bytes.Num();)
	{
		const int64 Size = Read32(Bytes, At + 4);
		const int32 Body = At + 8;
		if (Body + Size > Bytes.Num())
		{
			break;
		}
		if (IsTag(Bytes, At, "fmt ") && Size >= 16)
		{
			bFormat = Read16(Bytes, Body) == 1 && Read16(Bytes, Body + 2) == 1 && Read16(Bytes, Body + 14) == 16;
			Rate = int32(Read32(Bytes, Body + 4));
		}
		else if (IsTag(Bytes, At, "data"))
		{
			if (!bFormat || Rate <= 0 || (ExpectedRate != 0 && Rate != ExpectedRate))
			{
				OutError = FString::Printf(TEXT("%s: not 16-bit mono PCM at %d Hz"), *Path,
					ExpectedRate != 0 ? ExpectedRate : Rate);
				return false;
			}
			OutSamples.SetNumUninitialized(int32(Size / 2));
			FMemory::Memcpy(OutSamples.GetData(), Bytes.GetData() + Body, OutSamples.Num() * sizeof(int16));
			OutRate = Rate;
			return true;
		}
		At = Body + int32(Size) + int32(Size & 1);
	}
	OutError = Path + TEXT(": no sound in it");
	return false;
}
