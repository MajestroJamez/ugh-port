// The game's replays: each level's kept (the best), saved (F5), watched.
#include "Misc/FileHelper.h"
#include "UghEffects.h"
#include "UghGameMode.h"
#include "UghImpacts.h"
#include "UghSpeaker.h"

void AUghGameMode::OnLevelEnded(const TSharedPtr<FUghReplay>& Ended)
{
	const int32 Players = Ended->Players(), Level = Ended->Level();
	Ended->Label(Level < Passwords.LevelCount(Players) ? Passwords.Get(Players, Level) : FString(), Profile.Scores.LastName);
	LastLevel = Ended;
	LastLevelSaved.Reset();
	bLastLevelBest = Replays.IsWritable() && Replays.OfferBest(Ended);
	const ugh_replay_info& Info = Ended->GetInfo();
	UE_LOG(LogTemp, Display, TEXT("UGH replay of level %d: %s, %u points in %s, %d attempts, %d steps, %d keys%s"), Level + 1,
		Ended->IsDone() ? TEXT("done") : TEXT("not done"), Info.points, *FUghReplay::Clock(Ended->Seconds()), Info.attempts,
		Info.steps, Info.input_count, bLastLevelBest ? TEXT(", the best") : TEXT(""));
	if (bShooting && !Shot.GetSaveReplay().IsEmpty())
	{
		const bool bSaved = FFileHelper::SaveArrayToFile(Ended->Bytes(), *Shot.GetSaveReplay());
		UE_LOG(LogTemp, Display, TEXT("UGH shot: the replay %s %s"), *Shot.GetSaveReplay(),
			bSaved ? TEXT("saved") : TEXT("NOT saved"));
	}
}

void AUghGameMode::SayReplay(const FString& Text)
{
	ReplayNotice = Text;
	++ReplayNoticeCount;
}

void AUghGameMode::SaveLastLevel()
{
	if (!LastLevel)
	{
		SayReplay(TEXT("No level has ended yet: no replay to save"));
		return;
	}
	if (!LastLevelSaved.IsEmpty())
	{
		SayReplay(FString::Printf(TEXT("Saved already: %s"), *LastLevelSaved));
		return;
	}
	FString Error;
	LastLevelSaved = Replays.Save(LastLevel, FUghReplays::EKind::Saved, Error);
	SayReplay(LastLevelSaved.IsEmpty() ? FString::Printf(TEXT("Replay not saved: %s"), *Error)
		: FString::Printf(TEXT("Replay saved: %s"), *LastLevelSaved));
}

bool AUghGameMode::StartWatching(const TSharedPtr<FUghReplay>& Replay)
{
	if (!Replay || !Simulation.Watch(*Replay))
	{
		SayReplay(TEXT("This game has not the level of the replay"));
		return false;
	}
	const ugh_replay_info& Info = Replay->GetInfo();
	Watched = Replay;
	Speaker->GetPlayer().OnNewGame();
	bInMenu = false;
	StartKey = FKey();
	IntroLevel = -1;
	Playing = { Info.start.players, Info.start.difficulty, Info.start.level };
	PlayedLevel = -1;
	Controls.Reset();
	Effects->Clear();
	Impacts.Reset();
	UE_LOG(LogTemp, Display, TEXT("UGH watching a replay: %s, level %d, %u points in %s%s"),
		Info.start.players == 2 ? TEXT("team") : TEXT("one player"), Info.start.level + 1, Info.points,
		*FUghReplay::Clock(Replay->Seconds()),
		ugh_replay_compare_logic(Replay->GetHandle(), Simulation.GetLogic()) ? TEXT(" (made by another version)") : TEXT(""));
	return true;
}
