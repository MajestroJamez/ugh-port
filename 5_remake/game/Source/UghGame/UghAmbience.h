// The sounds of the world around the play, made as they play.
#pragma once

#include "CoreMinimal.h"
#include "UghMood.h"

/**
 * The ambience (step 29b), synthesized sample by sample - no recordings, nothing to download or package: the sea (a
 * low bed and waves rolling in and washing out, bigger and closer together in the storm), the jungle (birds of five
 * kinds by day, fewer in the evening; crickets, a katydid, frogs and now and then an owl at dusk and at night), the
 * wind (gusts with a whistle; a faint breeze over the open sea) and the rain (a hiss and drops) in the storm, the
 * campfires and torches crackling where they are (a voice each for at most FireVoices of them, the loudest; quieter
 * the farther from the ears, panned left or right), splashes (a passenger flung into the sea, a copter falling in,
 * its foam coming up) and the lianas rustling where a copter pushes them. The mood (FUghMood::EKind) chooses the
 * layers, the place (the menu's stone, the archipelago, the flight, the play) how loud each is, the presence how much
 * of it all is heard (the picture's fade: silent in the black). Stereo, -1 .. 1; deterministic (its own random
 * numbers). The ears are the camera's, moved towards the plane of the play up to EarsFromPlane in front of it (the
 * play's camera is 113 m away: from there every fire would sound alike).
 */
class FUghAmbience
{
public:
	enum class EPlace : uint8 { Menu, Isles, Flight, Play };
	enum class ESource : uint8 { Campfire, Torch };
	enum class ESplash : uint8 { Plunge, Dunk, Boil };

	static constexpr int32 FireVoices = 6, SplashVoices = 4, BirdVoices = 3, FrogVoices = 2, Crickets = 4, Drops = 8;
	/** The ears: at most this far (units) in front of the plane of the play, on the camera's line of sight. */
	static constexpr double EarsFromPlane = 1500;
	/** A fire is as loud as it is this near the ears (units), quieter farther; a splash, louder, this near. */
	static constexpr double FireNear = 800, SplashNear = 1400;
	/** All of it this loud: under the music (the original's, about -31 dBFS), the day about -38 dBFS, the storm -31. */
	static constexpr float Master = 0.8f;

	/** A fire in the world. */
	struct FFire
	{
		FVector Place = FVector::ZeroVector;
		ESource Kind = ESource::Campfire;
		bool bBurning = true;
	};

	explicit FUghAmbience(int32 InSampleRate = 49716) { SetSampleRate(InSampleRate); }
	void SetSampleRate(int32 InSampleRate);
	int32 GetSampleRate() const { return SampleRate; }

	/** What is around: the mood's layers, how loud the place has them, how much is seen (0 black .. 1). */
	void SetScene(FUghMood::EKind InMood, EPlace InPlace, float InPresence);
	/** The camera: where it is and where it looks (its X forward, Y right). */
	void SetListener(const FVector& Location, const FRotator& Rotation);
	/** The level's fires (any number; the loudest FireVoices are heard). */
	void SetFires(TConstArrayView<FFire> InFires);
	/** A splash at `Place` (the world), `Scale` 1 as usual. */
	void Splash(ESplash Kind, const FVector& Place, double Scale = 1);
	/** The lianas rustling: how much (0 .. 1) and where. */
	void SetRustle(float Strength, const FVector& Place);

	/** The next `Left.Num()` (= `Right.Num()`) samples, written. */
	void Render(TArrayView<float> Left, TArrayView<float> Right);

	/** The fires heard now (their voices sounding), for a look at it. */
	int32 FireVoicesSounding() const;
	/** Whether fire `Source` (of SetFires) has a voice sounding. */
	bool IsFireHeard(int32 Source) const;
	/** The splashes sounding now. */
	int32 SplashesSounding() const;
	/** The ears now (the world). */
	FVector Ears() const;
	/** How loud a source at `Where` sounds (0 .. 1) for its `Near`, and where (-1 left .. 1 right). */
	void Locate(const FVector& Where, double Near, float& OutGain, float& OutPan) const;

	/** A state-variable filter (Simper's): low, band (unity at its peak) and high pass at once. */
	struct FFilter
	{
		float Ic1 = 0, Ic2 = 0, A1 = 1, A2 = 0, A3 = 0, K = 1.4142f;
		float Low = 0, Band = 0, High = 0;
		void Set(float Hz, float Q, float Rate);
		void Step(float In);
	};
	/** Fast random numbers (xorshift). */
	struct FRandom
	{
		uint32 State = 1;
		float Signed() { State ^= State << 13; State ^= State >> 17; State ^= State << 5; return int32(State) * 4.656613e-10f; }
		float Unit() { return Signed() * 0.5f + 0.5f; }
	};

private:
	/** The frames rendered with the same settings of the filters and envelopes. */
	static constexpr int32 Block = 64;

	/** Equal power: a pan's gains of the left and the right. */
	struct FPan
	{
		float L = 0.7071f, R = 0.7071f;
		static FPan Of(float Pan);
	};
	struct FWave   // of the sea
	{
		FRandom Random;
		FFilter Filter;
		float Time = 0, Rise = 2, Wash = 3, Rest = 1, Size = 1, Gain = 0, Hz = 300;
		FPan Pan;
	};
	struct FCall   // a bird's, an owl's, a frog's
	{
		int32 Kind = -1;   // -1 silent
		int32 Notes = 1;
		float Time = 0, Note = 0.1f, Gap = 0, Phase = 0, Phase2 = 0, Rate = 0, Pitch = 1, Gain = 0, Damp = 1, Lp = 0;
		FPan Pan;
	};
	struct FCricket
	{
		float Hz = 4600, Period = 0.5f, Time = 0, Phase = 0, Gain = 0.01f;
		int32 Pulses = 4;
		FPan Pan;
	};
	struct FDrop
	{
		FFilter Filter;
		float Burst = 0, Size = 0;
		FPan Pan;
	};
	struct FFireVoice
	{
		int32 Source = INDEX_NONE;   // in Fires
		bool bTorch = false;
		FRandom Random;
		FFilter Roar, Crackle;
		float Gain = 0, Target = 0, Flicker = 1, FlickerTo = 1, FlickerIn = 0, Clicks = 0, ClickIn = 0, Env = 0,
			Decay = 0.9f, Big = 0;
		FPan Pan;
	};
	struct FSplashVoice
	{
		static constexpr int32 BubbleCount = 8;
		ESplash Kind = ESplash::Plunge;
		bool bOn = false;
		FFilter Slap, Wash;
		float Time = 0, Length = 1, Size = 1, Phase = 0;
		float BubbleAt[BubbleCount] = {}, BubbleHz[BubbleCount] = {}, BubblePhase[BubbleCount] = {};
		FPan Pan;
	};

	/** The layers' levels the mood and the place want (the levels heard move towards them). */
	struct FLevels
	{
		float Sea = 0, Waves = 0, Wind = 0, Rain = 0, Birds = 0, Crickets = 0, Frogs = 0, Owl = 0, Fires = 1;
	};
	static FLevels LevelsOf(FUghMood::EKind Mood, EPlace Place);

	void ControlBlock(float Seconds);
	void AssignFires();
	void StartWave(FWave& Wave, bool bFirst);
	void StartCall(FCall& Call, int32 Kind);
	/** What makes white noise filtered to a band `BandHz` wide about as loud as unfiltered (RMS 1). */
	float NoiseGain(float BandHz) const;

	int32 SampleRate = 49716;
	float Dt = 1.f / 49716;
	FUghMood::EKind Mood = FUghMood::EKind::Day;
	EPlace Place = EPlace::Menu;
	float Presence = 0, Heard = 0;   // wanted, smoothed
	FLevels Want, Now;
	FVector Listener = FVector::ZeroVector;
	FRotator Looking = FRotator::ZeroRotator;
	TArray<FFire> Fires;
	float RustleWant = 0, Rustle = 0, RustleGain = 0, RustleFlick = 0;
	FPan RustlePan;

	FRandom Random, SplashRandom, RustleRandom;   // (each its own: one splash more, the rest the same)
	FFilter SeaL, SeaR, WindL, WindR, Whistle, RainL, RainR, RainLpL, RainLpR, RustleL, RustleR;
	float SeaTime = 0, Swell = 1, Gust = 0.5f, GustTo = 0.5f, GustIn = 0;
	float SeaGain = 0, WindGain = 0, WhistleGain = 0, RainGain = 0, RustleNoise = 0;   // of the block
	FWave Waves[2];
	FCall Calls[BirdVoices + FrogVoices];
	float BirdIn = 1, FrogIn = 2, OwlIn = 20;
	FCricket CricketList[Crickets];
	float KatydidPhase = 0, KatydidTime = 0;
	FDrop DropList[Drops];
	int32 NextDrop = 0;
	FFireVoice FireList[FireVoices];
	FSplashVoice SplashList[SplashVoices];
	int32 BlockLeft = 0;
};
