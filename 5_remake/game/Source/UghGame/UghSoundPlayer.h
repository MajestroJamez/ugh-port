// When the game plays which sound.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghAmbience.h"
#include "UghMixer.h"
#include "UghSounds.h"

/**
 * When the game plays which sound, as the original did: the menu's music in the menu; in a game the effects of the
 * logic's events (FUghSounds::Cues) and the level's music from the start of the play; at its end the effects stop
 * and the music fades out as the picture does (over the original's 1.7 s); a lost game plays its jingle, then the
 * menu's music again; after the last level the ending's music plays in the menu. The sounds go to the mixer; the
 * ambience (FUghAmbience, synthesized) goes with them in stereo, which a speaker (AUghSpeaker) plays; without the
 * files nothing is heard.
 */
class FUghSoundPlayer
{
public:
	/** Reads the sounds of assets/sound and starts the menu's music. */
	void Load(const FString& SoundDir);

	/**
	 * The menu after a game that ended with `Result` (UGH_LOGIC_GAME_OVER: a jingle, UGH_LOGIC_ALL_LEVELS_DONE: the
	 * ending's music, UGH_LOGIC_REPLAY_OVER: a replay watched, the menu's music).
	 */
	void OnGameEnd(int32 Result);
	/** A game starts: the menu's music fades out. */
	void OnNewGame();
	/** An event of the logic. */
	void OnEvent(const ugh_logic_event& Event);
	/** The view after the steps of a frame: the play's start and end. */
	void OnView(const ugh_logic_view& View);

	/** The volumes in percent (FUghSettings): of everything, of the music, of the effects, of the ambience. */
	void SetVolumes(int32 Volume, int32 Music, int32 Effects, int32 Ambience = 100);
	float GetAmbienceVolume() const { return AmbienceVolume; }

	/** The next frames of the stream, left and right interleaved: the mixer's in the middle, the ambience around. */
	void MixStereo(TArrayView<int16> Out);

	FUghMixer& GetMixer() { return Mixer; }
	FUghAmbience& GetAmbience() { return Ambience; }
	const FUghSounds& GetSounds() const { return Sounds; }

private:
	/** The original's fade of the music: 0x11 steps of its 10 Hz timer. */
	static constexpr double MusicFadeSeconds = 1.7;

	void PlayMusic(const TCHAR* Name, int32 DelaySamples = 0);
	void FadeOutMusic();

	FUghSounds Sounds;
	FUghMixer Mixer;
	FUghAmbience Ambience;
	float AmbienceVolume = 1;
	TArray<int16> Mono;
	TArray<float> AmbienceLeft, AmbienceRight;
	int32 Phase = UGH_LOGIC_PHASE_START;
	int32 Fade = 0;
};
