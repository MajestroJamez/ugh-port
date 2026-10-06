#include "UghMixer.h"

void FUghMixer::PlayEffect(const TArray<int16>& Sound, int32 Owner, bool bLoop)
{
	FEffect* Channel = &Effects[0];
	for (FEffect& Effect : Effects)
	{
		if (!Effect.Sound)
		{
			Channel = &Effect;
			break;
		}
		if (Effect.Started < Channel->Started)
		{
			Channel = &Effect;
		}
	}
	*Channel = FEffect{ &Sound, 0, Owner, bLoop, ++EffectsStarted };
}

void FUghMixer::StopLoop(const TArray<int16>& Sound, int32 Owner)
{
	for (FEffect& Effect : Effects)
	{
		if (Effect.Sound == &Sound && Effect.Owner == Owner && Effect.bLoop)
		{
			Effect = FEffect();
		}
	}
}

void FUghMixer::StopEffects()
{
	for (FEffect& Effect : Effects)
	{
		Effect = FEffect();
	}
}

int32 FUghMixer::EffectCount() const
{
	int32 Count = 0;
	for (const FEffect& Effect : Effects)
	{
		Count += Effect.Sound ? 1 : 0;
	}
	return Count;
}

void FUghMixer::PlayMusic(const TArray<int16>& Sound, int32 DelaySamples)
{
	Music = FMusic{ &Sound, 0, FMath::Max(0, DelaySamples) };
}

void FUghMixer::FadeOutMusic(int32 Samples)
{
	if (Music.Sound && Music.FadeLength == 0)
	{
		Music.FadeLength = Music.FadeLeft = FMath::Max(1, Samples);
	}
}

float FUghMixer::NextMusicSample()
{
	if (!Music.Sound || Music.Sound->IsEmpty())
	{
		return 0;
	}
	if (Music.Delay > 0)
	{
		--Music.Delay;
		return 0;
	}
	float Sample = (*Music.Sound)[Music.Position];
	Music.Position = (Music.Position + 1) % Music.Sound->Num();
	if (Music.FadeLength > 0)
	{
		Sample *= float(Music.FadeLeft) / Music.FadeLength;
		if (--Music.FadeLeft == 0)
		{
			StopMusic();
		}
	}
	return Sample;
}

void FUghMixer::Mix(TArrayView<int16> Out)
{
	for (int16& Sample : Out)
	{
		float Sum = NextMusicSample() * MusicVolume;
		float EffectsSum = 0;
		for (FEffect& Effect : Effects)
		{
			if (!Effect.Sound)
			{
				continue;
			}
			if (Effect.Position >= Effect.Sound->Num())
			{
				if (!Effect.bLoop || Effect.Sound->IsEmpty())
				{
					Effect = FEffect();
					continue;
				}
				Effect.Position = 0;
			}
			EffectsSum += (*Effect.Sound)[Effect.Position++];
		}
		Sample = int16(FMath::Clamp(Sum + EffectsSum * EffectsVolume, -32768.f, 32767.f));
	}
}

void FUghMixer::SetVolumes(float InMusicVolume, float InEffectsVolume)
{
	MusicVolume = FMath::Clamp(InMusicVolume, 0.f, 1.f);
	EffectsVolume = FMath::Clamp(InEffectsVolume, 0.f, 1.f);
}
