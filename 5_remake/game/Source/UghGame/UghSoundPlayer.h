// When the game plays which sound.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghMixer.h"
#include "UghSounds.h"

/**
 * When the game plays which sound, as the original did: the menu's music in the menu; in a game the effects of the
 * logic's events (FUghSounds::Cues) and the level's music from the start of the play; at its end the effects stop
 * and the music fades out as the picture does (over the original's 1.7 s); a lost game plays its jingle, then the
 * menu's music again; after the last level the ending's music plays in the menu. The sounds go to the mixer, which
 * a speaker (AUghSpeaker) plays; without the files nothing is heard.
 */
class FUghSoundPlayer
{
public:
	/** The volume changes in steps of a tenth. */
	static constexpr int32 VolumeSteps = 10;

	/** Reads the sounds of assets/sound and starts the menu's music. */
	void Load(const FString& SoundDir);

	/** The menu after a game that ended with `Result` (UGH_LOGIC_GAME_OVER, UGH_LOGIC_ALL_LEVELS_DONE). */
	void OnGameEnd(int32 Result);
	/** A game starts: the menu's music fades out. */
	void OnNewGame();
	/** An event of the logic. */
	void OnEvent(const ugh_logic_event& Event);
	/** The view after the steps of a frame: the play's start and end. */
	void OnView(const ugh_logic_view& View);

	/** Louder (+1) or quieter (-1) by a step. */
	void ChangeVolume(int32 Steps);
	/** The volume in percent. */
	int32 GetVolumePercent() const { return FMath::RoundToInt(Mixer.GetVolume() * 100); }

	FUghMixer& GetMixer() { return Mixer; }
	const FUghSounds& GetSounds() const { return Sounds; }

private:
	/** The original's fade of the music: 0x11 steps of its 10 Hz timer. */
	static constexpr double MusicFadeSeconds = 1.7;

	void PlayMusic(const TCHAR* Name, int32 DelaySamples = 0);
	void FadeOutMusic();

	FUghSounds Sounds;
	FUghMixer Mixer;
	int32 Phase = UGH_LOGIC_PHASE_START;
	int32 Fade = 0;
};
