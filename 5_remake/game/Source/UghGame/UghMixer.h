// Mixes the sounds that play into one stream.
#pragma once

#include "CoreMinimal.h"

/**
 * The sounds that play now, mixed into one 16-bit stream (the samples of FUghSounds, all at one rate): the music (one
 * piece, repeated, it may fade out or wait before it starts) and up to Channels effects, like the original's four
 * effect channels (a fifth takes the place of the one that started first). An effect plays once or repeats until it is
 * stopped by its owner (the entity of the logic). The volume scales everything. Unlike the original, the effects do
 * not take the music's voices: both are heard.
 */
class FUghMixer
{
public:
	static constexpr int32 Channels = 4;

	/** An effect once (Owner INDEX_NONE) or repeated until StopLoop(Sound, Owner). */
	void PlayEffect(const TArray<int16>& Sound, int32 Owner = INDEX_NONE, bool bLoop = false);
	void StopLoop(const TArray<int16>& Sound, int32 Owner);
	void StopEffects();
	int32 EffectCount() const;

	/** The music from its start after `DelaySamples` of silence, repeated; it replaces the music that plays. */
	void PlayMusic(const TArray<int16>& Sound, int32 DelaySamples = 0);
	/** The music fades out in `Samples` (and stops); already fading, it goes on as it was. */
	void FadeOutMusic(int32 Samples);
	void StopMusic() { Music = FMusic(); }
	/** The music that plays (or waits for its start), nullptr none. */
	const TArray<int16>* GetMusic() const { return Music.Sound; }

	/** 0 silent .. 1 as the sounds are. */
	void SetVolume(float InVolume) { Volume = FMath::Clamp(InVolume, 0.f, 1.f); }
	float GetVolume() const { return Volume; }

	/** The next `Out.Num()` samples of the stream. */
	void Mix(TArrayView<int16> Out);

private:
	struct FEffect
	{
		const TArray<int16>* Sound = nullptr;
		int32 Position = 0;
		int32 Owner = INDEX_NONE;
		bool bLoop = false;
		uint64 Started = 0;   // which effect started first
	};
	struct FMusic
	{
		const TArray<int16>* Sound = nullptr;
		int32 Position = 0;
		int32 Delay = 0;
		int32 FadeLength = 0;   // 0: not fading
		int32 FadeLeft = 0;
	};

	/** One sample of the music (the fade applied), or 0; moves on. */
	float NextMusicSample();

	FEffect Effects[Channels];
	FMusic Music;
	uint64 EffectsStarted = 0;
	float Volume = 1.f;
};
