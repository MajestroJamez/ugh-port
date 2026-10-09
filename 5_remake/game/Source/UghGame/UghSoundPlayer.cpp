#include "UghSoundPlayer.h"

void FUghSoundPlayer::Load(const FString& SoundDir)
{
	Mixer.StopEffects();   // they point into the sounds
	Mixer.StopMusic();
	Sounds.Load(SoundDir);
	PlayMusic(FUghSounds::MenuMusic);
}

void FUghSoundPlayer::OnGameEnd(int32 Result)
{
	Mixer.StopEffects();
	Phase = UGH_LOGIC_PHASE_START;
	if (Result == UGH_LOGIC_REPLAY_OVER)
	{
		PlayMusic(FUghSounds::MenuMusic);   // a replay watched: back to the menu, no jingle
		return;
	}
	if (Result == UGH_LOGIC_ALL_LEVELS_DONE)
	{
		PlayMusic(FUghSounds::EndingMusic);
		return;
	}
	const TArray<int16>* Jingle = Sounds.Find(FUghSounds::GameOver);
	if (Jingle)
	{
		Mixer.PlayEffect(*Jingle);
	}
	PlayMusic(FUghSounds::MenuMusic, Jingle ? Jingle->Num() : 0);
}

void FUghSoundPlayer::OnNewGame()
{
	FadeOutMusic();
}

void FUghSoundPlayer::OnEvent(const ugh_logic_event& Event)
{
	const FUghSounds::FCue* Cue = FUghSounds::CueOf(Event.kind);
	const TArray<int16>* Sound = Cue ? Sounds.Find(Cue->Name) : nullptr;
	if (!Sound)
	{
		return;
	}
	switch (Cue->Action)
	{
	case FUghSounds::EAction::Play: Mixer.PlayEffect(*Sound); break;
	case FUghSounds::EAction::Loop: Mixer.PlayEffect(*Sound, Event.entity, true); break;
	case FUghSounds::EAction::Stop: Mixer.StopLoop(*Sound, Event.entity); break;
	}
}

void FUghSoundPlayer::OnView(const ugh_logic_view& View)
{
	const bool bPlay = View.phase == UGH_LOGIC_PHASE_PLAY;
	if (bPlay && Phase != UGH_LOGIC_PHASE_PLAY)
	{
		PlayMusic(FUghSounds::GameMusic);
	}
	else if (bPlay && View.fade < Fade)
	{
		FadeOutMusic();   // the play fades out: the level is over
	}
	else if (!bPlay && Phase == UGH_LOGIC_PHASE_PLAY)
	{
		Mixer.StopEffects();
		FadeOutMusic();
	}
	Phase = View.phase;
	Fade = View.fade;
}

void FUghSoundPlayer::SetVolumes(int32 Volume, int32 Music, int32 Effects)
{
	const float All = Volume / 100.f;
	Mixer.SetVolumes(All * Music / 100.f, All * Effects / 100.f);
}

void FUghSoundPlayer::PlayMusic(const TCHAR* Name, int32 DelaySamples)
{
	if (const TArray<int16>* Music = Sounds.Find(Name))
	{
		Mixer.PlayMusic(*Music, DelaySamples);
	}
	else
	{
		Mixer.StopMusic();
	}
}

void FUghSoundPlayer::FadeOutMusic()
{
	Mixer.FadeOutMusic(FMath::RoundToInt(MusicFadeSeconds * Sounds.GetSampleRate()));
}
