#include "UghAmbience.h"

namespace
{
	constexpr float TwoPi = 2.f * PI;

	/** How much of the way to its target a value moves in `Seconds` with time constant `Tau`. */
	float Toward(float Seconds, float Tau)
	{
		return 1.f - FMath::Exp(-Seconds / Tau);
	}

	float Sine(float Phase)
	{
		return FMath::Sin(TwoPi * Phase);
	}

	void Wrap(float& Phase)
	{
		Phase -= FMath::FloorToFloat(Phase);
	}

	/** The bird calls: how long a note is and the gap after it (seconds). */
	struct FCallShape
	{
		float Note, Gap;
	};
	enum ECall : int32 { Chirps, Trill, TwoNote, Whoop, Squawk, BirdKinds, Owl = BirdKinds, Frog };
	const FCallShape CallShapes[] = { { 0.07f, 0.06f }, { 0.7f, 0.f }, { 0.22f, 0.08f }, { 0.45f, 0.25f },
		{ 0.22f, 0.12f }, { 0.35f, 0.25f }, { 0.45f, 0.f } };
}

void FUghAmbience::FFilter::Set(float Hz, float Q, float Rate)
{
	const float G = FMath::Tan(PI * FMath::Clamp(Hz, 10.f, 0.45f * Rate) / Rate);
	K = 1.f / Q;
	A1 = 1.f / (1.f + G * (G + K));
	A2 = G * A1;
	A3 = G * A2;
}

void FUghAmbience::FFilter::Step(float In)
{
	const float V3 = In - Ic2;
	const float V1 = A1 * Ic1 + A2 * V3;
	const float V2 = Ic2 + A2 * Ic1 + A3 * V3;
	Ic1 = 2 * V1 - Ic1;
	Ic2 = 2 * V2 - Ic2;
	Low = V2;
	Band = K * V1;
	High = In - K * V1 - V2;
}

FUghAmbience::FPan FUghAmbience::FPan::Of(float Pan)
{
	const float Angle = (FMath::Clamp(Pan, -1.f, 1.f) + 1.f) * PI / 4;
	return FPan{ FMath::Cos(Angle), FMath::Sin(Angle) };
}

void FUghAmbience::SetSampleRate(int32 InSampleRate)
{
	SampleRate = FMath::Max(8000, InSampleRate);
	Dt = 1.f / SampleRate;
	Random.State = 0x2B9u;
	SplashRandom.State = 0x5B1A5u;
	RustleRandom.State = 0x7E57u;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Waves); ++Index)
	{
		Waves[Index].Random.State = 0x5EA0u + Index * 977;
		StartWave(Waves[Index], Index == 0);
	}
	for (int32 Index = 0; Index < FireVoices; ++Index)
	{
		FireList[Index].Random.State = 0xF1E0u + Index * 7919;
	}
	for (int32 Index = 0; Index < Crickets; ++Index)
	{
		FCricket& Cricket = CricketList[Index];
		Cricket.Hz = 4400 + 700 * Random.Unit();
		Cricket.Period = 0.35f + 0.55f * Random.Unit();
		Cricket.Time = Cricket.Period * Random.Unit();
		Cricket.Pulses = 4 + int32(Random.Unit() * 4);
		Cricket.Gain = 0.014f + 0.02f * Random.Unit();
		Cricket.Pan = FPan::Of(-0.8f + 1.6f * (Index + 0.5f * Random.Unit()) / Crickets);
	}
	SeaL.Set(380, 0.7f, SampleRate);
	SeaR.Set(380, 0.7f, SampleRate);
	RainL.Set(900, 0.7f, SampleRate);
	RainR.Set(900, 0.7f, SampleRate);
	RainLpL.Set(7000, 0.7f, SampleRate);
	RainLpR.Set(7000, 0.7f, SampleRate);
	RustleL.Set(3800, 0.7f, SampleRate);
	RustleR.Set(4200, 0.7f, SampleRate);
	BlockLeft = 0;
}

FUghAmbience::FLevels FUghAmbience::LevelsOf(FUghMood::EKind Mood, EPlace Place)
{
	using EKind = FUghMood::EKind;
	FLevels Levels;
	Levels.Sea = Levels.Waves = 1;
	Levels.Wind = 0.12f;   // a breeze
	switch (Mood)
	{
	case EKind::Day: Levels.Birds = 1; break;
	case EKind::Evening: Levels.Birds = 0.6f; Levels.Crickets = 0.3f; Levels.Frogs = 0.2f; break;
	case EKind::Dusk: Levels.Birds = 0.25f; Levels.Crickets = 0.7f; Levels.Frogs = 0.6f; Levels.Owl = 0.3f; break;
	case EKind::Night: Levels.Sea = 0.85f; Levels.Crickets = 1; Levels.Frogs = 1; Levels.Owl = 1; break;
	case EKind::Storm: Levels.Sea = 1.3f; Levels.Waves = 1.5f; Levels.Wind = 1; Levels.Rain = 1; Levels.Birds = 0.05f; break;
	}
	// the place: the open sea louder over it, the jungle nearest on the menu's stone, the breeze but in the cave
	static constexpr float Sea[] = { 0.9f, 1.f, 1.f, 0.75f }, Jungle[] = { 0.9f, 0.35f, 0.5f, 0.7f };
	const int32 At = static_cast<int32>(Place);
	Levels.Sea *= Sea[At];
	Levels.Waves *= Sea[At];
	for (float* Layer : { &Levels.Birds, &Levels.Crickets, &Levels.Frogs, &Levels.Owl })
	{
		*Layer *= Jungle[At];
	}
	if (Mood != EKind::Storm && Place == EPlace::Play)
	{
		Levels.Wind *= 0.3f;
	}
	return Levels;
}

void FUghAmbience::SetScene(FUghMood::EKind InMood, EPlace InPlace, float InPresence)
{
	Mood = InMood;
	Place = InPlace;
	Presence = FMath::Clamp(InPresence, 0.f, 1.f);
	Want = LevelsOf(Mood, Place);
}

void FUghAmbience::SetListener(const FVector& Location, const FRotator& Rotation)
{
	Listener = Location;
	Looking = Rotation;
}

void FUghAmbience::SetFires(TConstArrayView<FFire> InFires)
{
	Fires = TArray<FFire>(InFires);
}

FVector FUghAmbience::Ears() const
{
	const FVector Forward = Looking.Vector();
	if (FMath::Abs(Forward.Y) > 1e-3)
	{
		const double Along = -Listener.Y / Forward.Y;   // to the plane of the play (world Y 0)
		if (Along > 0)
		{
			return Listener + Forward * FMath::Clamp(Along - EarsFromPlane, 0.0, 30000.0);
		}
	}
	return Listener;
}

void FUghAmbience::Locate(const FVector& Where, double Near, float& OutGain, float& OutPan) const
{
	const FVector Way = Where - Ears();
	const double Distance = Way.Size();
	OutGain = float(FMath::Min(1.0, FMath::Pow(Near / FMath::Max(Distance, 1.0), 1.2)));
	OutPan = Distance < 1 ? 0.f
		: float(FMath::Clamp(FVector::DotProduct(Way / Distance, Looking.RotateVector(FVector::RightVector)), -0.9, 0.9));
}

void FUghAmbience::Splash(ESplash Kind, const FVector& Where, double Scale)
{
	FSplashVoice* Voice = &SplashList[0];
	for (FSplashVoice& Each : SplashList)
	{
		if (!Each.bOn)
		{
			Voice = &Each;
			break;
		}
		if (Each.Time > Voice->Time)
		{
			Voice = &Each;   // else the oldest
		}
	}
	float Gain = 0, Pan = 0;
	Locate(Where, SplashNear, Gain, Pan);
	static constexpr float Sizes[] = { 0.25f, 0.45f, 0.12f }, Lengths[] = { 1.4f, 2.2f, 1.2f };
	FSplashVoice Fresh;
	Fresh.Kind = Kind;
	Fresh.bOn = true;
	Fresh.Length = Lengths[static_cast<int32>(Kind)];
	Fresh.Size = Sizes[static_cast<int32>(Kind)] * FMath::Clamp(float(Scale), 0.3f, 2.f) * Gain;
	Fresh.Pan = FPan::Of(Pan);
	// bubbles coming up after it (a foam: only bubbles)
	const int32 Bubbles = Kind == ESplash::Plunge ? 4 : FSplashVoice::BubbleCount;
	for (int32 Index = 0; Index < FSplashVoice::BubbleCount; ++Index)
	{
		const bool bOn = Index < Bubbles;
		Fresh.BubbleAt[Index] = !bOn ? 99.f
			: Kind == ESplash::Boil ? 0.9f * SplashRandom.Unit() : (Kind == ESplash::Dunk ? 0.2f : 0.15f) + SplashRandom.Unit();
		Fresh.BubbleHz[Index] = (Kind == ESplash::Dunk ? 300 : 450) + 800 * SplashRandom.Unit();
	}
	Fresh.Slap.Set(3500, 0.7f, SampleRate);
	Fresh.Wash.Set(Kind == ESplash::Dunk ? 800.f : 1100.f, 0.7f, SampleRate);
	*Voice = Fresh;
}

void FUghAmbience::SetRustle(float Strength, const FVector& Where)
{
	RustleWant = FMath::Clamp(Strength, 0.f, 1.f);
	float Pan = 0;
	Locate(Where, FireNear, RustleGain, Pan);
	RustlePan = FPan::Of(Pan);
}

int32 FUghAmbience::FireVoicesSounding() const
{
	int32 Count = 0;
	for (const FFireVoice& Voice : FireList)
	{
		Count += Voice.Source != INDEX_NONE && Voice.Gain > 1e-3f ? 1 : 0;
	}
	return Count;
}

bool FUghAmbience::IsFireHeard(int32 Source) const
{
	for (const FFireVoice& Voice : FireList)
	{
		if (Voice.Source == Source && Voice.Gain > 1e-3f)
		{
			return true;
		}
	}
	return false;
}

int32 FUghAmbience::SplashesSounding() const
{
	int32 Count = 0;
	for (const FSplashVoice& Voice : SplashList)
	{
		Count += Voice.bOn ? 1 : 0;
	}
	return Count;
}

float FUghAmbience::NoiseGain(float BandHz) const
{
	return 1.732f / FMath::Sqrt(FMath::Clamp(PI * BandHz / SampleRate, 1e-5f, 1.f));
}

void FUghAmbience::StartWave(FWave& Wave, bool bFirst)
{
	const bool bStorm = Mood == FUghMood::EKind::Storm;
	FRandom& R = Wave.Random;
	Wave.Rise = (bStorm ? 1.0f : 1.5f) + 1.2f * R.Unit();
	Wave.Wash = (bStorm ? 1.8f : 2.2f) + 2.f * R.Unit();
	Wave.Rest = (bStorm ? 0.1f : 0.6f) + (bStorm ? 1.f : 2.5f) * R.Unit();
	Wave.Size = 0.55f + 0.45f * R.Unit();
	Wave.Pan = FPan::Of(R.Signed() * 0.6f);
	// the second wave starts half a wave later
	Wave.Time = bFirst ? 0 : -(Wave.Rise + Wave.Wash) * 0.5f;
}

void FUghAmbience::StartCall(FCall& Call, int32 Kind)
{
	FCall Fresh;
	Fresh.Kind = Kind;
	Fresh.Note = CallShapes[Kind].Note;
	Fresh.Gap = CallShapes[Kind].Gap;
	const float Near = Random.Unit();
	Fresh.Gain = 0.025f + 0.06f * Near * Near;
	Fresh.Pitch = 0.88f + 0.27f * Random.Unit();
	Fresh.Pan = FPan::Of(Random.Signed() * 0.85f);
	// the farther, the duller
	Fresh.Damp = Toward(1.f, 1.f / (TwoPi * (2500.f + 9000.f * Near * Near) * Dt));
	switch (Kind)
	{
	case Chirps: Fresh.Notes = 3 + int32(Random.Unit() * 4); break;
	case Trill: Fresh.Note = 0.5f + 0.4f * Random.Unit(); break;
	case TwoNote: Fresh.Notes = 2 + int32(Random.Unit() * 2); break;
	case Whoop: Fresh.Notes = 2; break;
	case Squawk: Fresh.Notes = 1 + int32(Random.Unit() * 2); Fresh.Gain *= 0.7f; break;
	case Owl: Fresh.Notes = 2; Fresh.Pitch = 0.92f + 0.16f * Random.Unit(); Fresh.Gain = 0.03f + 0.02f * Near; break;
	case Frog:
		Fresh.Note = 0.3f + 0.35f * Random.Unit();
		Fresh.Rate = 22 + 8 * Random.Unit();
		Fresh.Pitch = 0.75f + 0.6f * Random.Unit();
		Fresh.Gain = 0.02f + 0.025f * Near;
		break;
	default: break;
	}
	Call = Fresh;
}

void FUghAmbience::AssignFires()
{
	// the loudest fires that burn
	TArray<TPair<float, int32>, TInlineAllocator<32>> Loud;
	for (int32 Index = 0; Index < Fires.Num(); ++Index)
	{
		if (Fires[Index].bBurning)
		{
			float Gain = 0, Pan = 0;
			Locate(Fires[Index].Place, FireNear, Gain, Pan);
			Loud.Add({ Fires[Index].Kind == ESource::Torch ? Gain * 0.7f : Gain, Index });
		}
	}
	Loud.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key > B.Key; });
	Loud.SetNum(FMath::Min(Loud.Num(), FireVoices));
	auto Chosen = [&Loud](int32 Source) { return Loud.ContainsByPredicate([Source](const TPair<float, int32>& Each) { return Each.Value == Source; }); };
	for (FFireVoice& Voice : FireList)
	{
		if (Voice.Source != INDEX_NONE && (!Fires.IsValidIndex(Voice.Source) || !Chosen(Voice.Source)))
		{
			Voice.Target = 0;   // fades out, then free
			if (Voice.Gain < 1e-4f)
			{
				Voice.Source = INDEX_NONE;
			}
		}
	}
	for (const TPair<float, int32>& Each : Loud)
	{
		FFireVoice* Voice = nullptr;
		for (FFireVoice& Candidate : FireList)
		{
			if (Candidate.Source == Each.Value)
			{
				Voice = &Candidate;
				break;
			}
		}
		if (!Voice)
		{
			for (FFireVoice& Candidate : FireList)
			{
				if (Candidate.Source == INDEX_NONE)
				{
					Voice = &Candidate;
					Voice->Source = Each.Value;
					Voice->Gain = 0;
					break;
				}
			}
		}
		if (Voice)
		{
			float Gain = 0, Pan = 0;
			Locate(Fires[Each.Value].Place, FireNear, Gain, Pan);
			Voice->Target = Gain;
			Voice->Pan = FPan::Of(Pan);
			Voice->bTorch = Fires[Each.Value].Kind == ESource::Torch;
		}
	}
}

void FUghAmbience::ControlBlock(float Seconds)
{
	const bool bStorm = Mood == FUghMood::EKind::Storm;
	Heard += (Presence - Heard) * Toward(Seconds, 0.5f);
	const float Move = Toward(Seconds, 1.5f);
	float* NowLevels = &Now.Sea;
	const float* WantLevels = &Want.Sea;
	for (int32 Layer = 0; Layer < int32(sizeof(FLevels) / sizeof(float)); ++Layer)
	{
		NowLevels[Layer] += (WantLevels[Layer] - NowLevels[Layer]) * Move;
	}

	// the sea: its bed swelling slowly, two waves rolling in and washing out
	SeaTime += Seconds;
	Swell = 0.75f + 0.25f * Sine(0.09f * SeaTime);
	SeaGain = Now.Sea * 0.02f * NoiseGain(380) * Swell;
	for (FWave& Wave : Waves)
	{
		Wave.Time += Seconds;
		if (Wave.Time >= Wave.Rise + Wave.Wash + Wave.Rest)
		{
			StartWave(Wave, true);
		}
		const float Low = bStorm ? 300.f : 220.f, High = bStorm ? 2600.f : 1600.f;
		float Shape = 0;
		if (Wave.Time > 0 && Wave.Time < Wave.Rise)
		{
			const float X = Wave.Time / Wave.Rise;
			Shape = X * X * (3 - 2 * X);
			Wave.Hz = Low + (High - Low) * Shape;
		}
		else if (Wave.Time >= Wave.Rise && Wave.Time < Wave.Rise + Wave.Wash)
		{
			const float Y = (Wave.Time - Wave.Rise) / Wave.Wash;
			Shape = (1 - Y) * (1 - Y);
			Wave.Hz = 500 + (High - 500) * (1 - Y);
		}
		Wave.Gain = Shape * Wave.Size * Now.Waves * 0.045f * NoiseGain(Wave.Hz);
		Wave.Filter.Set(Wave.Hz, 0.6f, SampleRate);
	}

	// the wind's gusts
	GustIn -= Seconds;
	if (GustIn <= 0)
	{
		GustTo = bStorm ? 0.2f + 0.8f * Random.Unit() : 0.1f + 0.4f * Random.Unit();
		GustIn = 0.8f + 2.5f * Random.Unit();
	}
	Gust += (GustTo - Gust) * Toward(Seconds, 0.7f);
	const float WindHz = 250 + 700 * Gust;
	WindL.Set(WindHz, 1.2f, SampleRate);
	WindR.Set(WindHz * 1.13f, 1.2f, SampleRate);
	Whistle.Set(700 + 900 * Gust, 22, SampleRate);
	WindGain = Now.Wind * 0.035f * NoiseGain(WindHz / 1.2f) * (0.25f + 0.75f * Gust * Gust);
	WhistleGain = Now.Wind * 0.012f * NoiseGain((700 + 900 * Gust) / 22) * Gust * Gust * Gust;

	// the rain: a hiss, drops
	RainGain = Now.Rain * 0.022f * NoiseGain(6000);
	if (Now.Rain > 0.01f && Random.Unit() < 70 * Now.Rain * Seconds)
	{
		FDrop& Drop = DropList[NextDrop];
		NextDrop = (NextDrop + 1) % Drops;
		Drop.Filter.Set(1500 + 3500 * Random.Unit(), 7, SampleRate);
		const float Size = Random.Unit();
		Drop.Size = 0.05f + 0.25f * Size * Size * Size;
		Drop.Burst = 1;
		Drop.Pan = FPan::Of(Random.Signed() * 0.9f);
	}

	// the jungle's calls
	auto Free = [this](int32 From, int32 To) -> FCall*
	{
		for (int32 Index = From; Index < To; ++Index)
		{
			if (Calls[Index].Kind < 0)
			{
				return &Calls[Index];
			}
		}
		return nullptr;
	};
	if (Now.Birds > 0.02f && (BirdIn -= Seconds) <= 0)
	{
		if (FCall* Call = Free(0, BirdVoices))
		{
			StartCall(*Call, FMath::Min(int32(Random.Unit() * float(BirdKinds)), int32(BirdKinds) - 1));
		}
		BirdIn = FMath::Min(30.f, -FMath::Loge(FMath::Max(Random.Unit(), 1e-3f)) * 1.4f / FMath::Max(Now.Birds, 0.05f));
	}
	if (Now.Owl > 0.05f && (OwlIn -= Seconds) <= 0)
	{
		if (FCall* Call = Free(0, BirdVoices))
		{
			StartCall(*Call, Owl);
		}
		OwlIn = (14 + 16 * Random.Unit()) / FMath::Max(Now.Owl, 0.3f);
	}
	if (Now.Frogs > 0.02f && (FrogIn -= Seconds) <= 0)
	{
		if (FCall* Call = Free(BirdVoices, BirdVoices + FrogVoices))
		{
			StartCall(*Call, Frog);
		}
		FrogIn = (1.5f + 4 * Random.Unit()) / FMath::Max(Now.Frogs, 0.2f);
	}
	for (FCall& Call : Calls)
	{
		if (Call.Kind >= 0 && Call.Time >= Call.Notes * (Call.Note + Call.Gap))
		{
			Call.Kind = -1;
		}
	}

	// the fires: which are heard, how they flicker, when they crackle
	AssignFires();
	for (FFireVoice& Voice : FireList)
	{
		if (Voice.Source == INDEX_NONE)
		{
			continue;
		}
		FRandom& R = Voice.Random;
		if ((Voice.FlickerIn -= Seconds) <= 0)
		{
			Voice.FlickerTo = 0.35f + 0.65f * R.Unit();
			Voice.FlickerIn = Voice.bTorch ? 0.04f + 0.06f * R.Unit() : 0.12f + 0.2f * R.Unit();
		}
		Voice.Flicker += (Voice.FlickerTo - Voice.Flicker) * Toward(Seconds, Voice.bTorch ? 0.03f : 0.08f);
		Voice.Roar.Set(Voice.bTorch ? 260.f : 170.f, 0.7f, SampleRate);
		if (Voice.Clicks <= 0 && R.Unit() < (Voice.bTorch ? 4.f : 10.f) * Seconds)
		{
			Voice.Clicks = 1 + int32(R.Unit() * 3);
			Voice.ClickIn = 0;
			Voice.Big = R.Unit() < 0.08f ? 1.f : 0.f;
		}
		Voice.Crackle.Set(Voice.Big > 0 ? 900.f : (Voice.bTorch ? 3200.f : 2600.f), 0.9f, SampleRate);
	}

	// the splashes' filters close as they wash out
	for (FSplashVoice& Splash : SplashList)
	{
		if (!Splash.bOn)
		{
			continue;
		}
		if (Splash.Time >= Splash.Length)
		{
			Splash.bOn = false;
			continue;
		}
		Splash.Slap.Set(600 + 3500 * FMath::Exp(-Splash.Time / 0.25f), 0.7f, SampleRate);
	}

	// the lianas
	Rustle += (RustleWant - Rustle) * Toward(Seconds, 0.08f);
	RustleNoise = 0.04f * NoiseGain(3800 / 0.7f) * FMath::Pow(Rustle, 1.5f) * RustleGain;
	if (Rustle > 0.02f && RustleRandom.Unit() < 50 * Rustle * Seconds)
	{
		RustleFlick = (0.3f + 0.7f * RustleRandom.Unit()) * Rustle * RustleGain;
	}
}

void FUghAmbience::Render(TArrayView<float> Left, TArrayView<float> Right)
{
	check(Left.Num() == Right.Num());
	const float BurstFade = FMath::Exp(-Dt / 0.0015f), ClickFade = FMath::Exp(-Dt / 0.003f);
	const float Slap = 1.f / 0.003f, Plunge = 1.f / 0.07f, DunkSlap = 1.f / 0.12f;
	const float CampfireRoar = NoiseGain(170) * 0.025f, TorchRoar = NoiseGain(260) * 0.014f;
	for (int32 Frame = 0; Frame < Left.Num(); ++Frame)
	{
		if (BlockLeft == 0)
		{
			ControlBlock(Block * Dt);
			BlockLeft = Block;
		}
		--BlockLeft;
		float L = 0, R = 0;

		// the sea
		SeaL.Step(Random.Signed());
		SeaR.Step(Random.Signed());
		L += SeaL.Low * SeaGain;
		R += SeaR.Low * SeaGain;
		for (FWave& Wave : Waves)
		{
			if (Wave.Gain > 1e-5f)
			{
				Wave.Filter.Step(Wave.Random.Signed());
				const float Value = Wave.Filter.Low * Wave.Gain;
				L += Value * Wave.Pan.L;
				R += Value * Wave.Pan.R;
			}
		}

		// the wind and the rain
		if (WindGain > 1e-5f)
		{
			const float Noise = Random.Signed();
			WindL.Step(Noise);
			WindR.Step(Random.Signed());
			Whistle.Step(Noise);
			L += WindL.Band * WindGain + Whistle.Band * WhistleGain * 0.8f;
			R += WindR.Band * WindGain + Whistle.Band * WhistleGain * 0.6f;
		}
		if (RainGain > 1e-5f)
		{
			RainL.Step(Random.Signed());
			RainR.Step(Random.Signed());
			RainLpL.Step(RainL.High);
			RainLpR.Step(RainR.High);
			L += RainLpL.Low * RainGain;
			R += RainLpR.Low * RainGain;
		}
		for (FDrop& Drop : DropList)
		{
			if (Drop.Burst > 1e-3f)
			{
				Drop.Filter.Step(Random.Signed() * Drop.Burst);
				Drop.Burst *= BurstFade;
				const float Value = Drop.Filter.Band * Drop.Size * Now.Rain;
				L += Value * Drop.Pan.L;
				R += Value * Drop.Pan.R;
			}
		}

		// the jungle
		for (FCall& Call : Calls)
		{
			if (Call.Kind < 0)
			{
				continue;
			}
			const float Period = Call.Note + Call.Gap;
			const int32 Note = FMath::FloorToInt32(Call.Time / Period);
			const float Within = Call.Time - Note * Period;
			Call.Time += Dt;
			const float U = Within / Call.Note;
			float Hz = 0, Amp = 0, Value = 0;
			if (U < 1)
			{
				const float Arc = FMath::Sin(PI * U);
				switch (Call.Kind)
				{
				case Chirps: Hz = 2400 + 1100 * U; Amp = Arc; break;
				case Trill: Hz = 3000 + 380 * Sine(27 * Call.Time); Amp = FMath::Sqrt(Arc); break;
				case TwoNote: Hz = (Note % 2 == 0 ? 1500.f : 1150.f) * (1 - 0.04f * U); Amp = FMath::Pow(Arc, 0.7f); break;
				case Whoop: Hz = 1100 * FMath::Exp(0.742f * U); Amp = Arc; break;
				case Squawk: Hz = 1700; Amp = FMath::Sqrt(Arc); break;
				case Owl: Hz = 380 * (1 - 0.06f * U); Amp = Arc * FMath::Sqrt(Arc); break;
				case Frog:
				{
					Call.Phase2 += Call.Rate * Dt;
					Wrap(Call.Phase2);
					Hz = 600;
					Amp = Arc * (Call.Phase2 < 0.35f ? FMath::Sin(PI * Call.Phase2 / 0.35f) : 0.f);
					break;
				}
				default: break;
				}
				Call.Phase += Hz * Call.Pitch * Dt;
				Wrap(Call.Phase);
				if (Call.Kind == Squawk)
				{
					Call.Phase2 += 230 * Dt;
					Wrap(Call.Phase2);
					Value = Sine(Call.Phase + 0.6f * Sine(Call.Phase2));
				}
				else if (Call.Kind == Frog)
				{
					Value = Sine(Call.Phase) + 0.5f * Sine(2 * Call.Phase) + 0.25f * Sine(3 * Call.Phase);
				}
				else
				{
					Value = Sine(Call.Phase) + (Call.Kind == Owl || Call.Kind == TwoNote ? 0.12f * Sine(2 * Call.Phase) : 0.f);
				}
				const float Layer = Call.Kind == Owl ? Now.Owl : Call.Kind == Frog ? Now.Frogs : Now.Birds;
				Value *= Amp * Call.Gain * Layer;
			}
			Call.Lp += Call.Damp * (Value - Call.Lp);
			L += Call.Lp * Call.Pan.L;
			R += Call.Lp * Call.Pan.R;
		}
		if (Now.Crickets > 1e-3f)
		{
			for (FCricket& Cricket : CricketList)
			{
				Cricket.Time += Dt;
				if (Cricket.Time >= Cricket.Period)
				{
					Cricket.Time -= Cricket.Period;
				}
				Cricket.Phase += Cricket.Hz * Dt;
				Wrap(Cricket.Phase);
				const int32 Pulse = FMath::FloorToInt32(Cricket.Time / 0.026f);
				const float Within = Cricket.Time - Pulse * 0.026f;
				if (Pulse < Cricket.Pulses && Within < 0.012f)
				{
					const float Value = Sine(Cricket.Phase) * FMath::Sin(PI * Within / 0.012f) * Cricket.Gain * Now.Crickets;
					L += Value * Cricket.Pan.L;
					R += Value * Cricket.Pan.R;
				}
			}
			// a katydid buzzing on and off
			KatydidTime += Dt;
			KatydidPhase += 4700 * Dt;
			Wrap(KatydidPhase);
			const float Buzz = FMath::Max(0.f, Sine(55 * KatydidTime));
			const float Value = Sine(KatydidPhase) * Buzz * Buzz * 0.012f * (0.5f + 0.5f * Sine(0.07f * KatydidTime)) *
				Now.Crickets;
			L += Value * 0.4f;
			R += Value * 0.8f;
		}

		// the fires
		for (FFireVoice& Voice : FireList)
		{
			if (Voice.Source == INDEX_NONE)
			{
				continue;
			}
			Voice.Gain += (Voice.Target - Voice.Gain) * 0.0015f;
			FRandom& Rand = Voice.Random;
			if (Voice.Clicks > 0 && (Voice.ClickIn -= Dt) <= 0)
			{
				const float Size = Rand.Unit();
				Voice.Env = Voice.Big > 0 ? 0.25f : 0.12f * (0.1f + 0.9f * Size * Size * Size * Size);
				Voice.Decay = FMath::Exp(-Dt / (Voice.Big > 0 ? 0.005f : 0.0006f + 0.002f * Rand.Unit()));
				Voice.Clicks -= 1;
				Voice.ClickIn = 0.002f + 0.006f * Rand.Unit();
			}
			const float Noise = Rand.Signed();
			Voice.Roar.Step(Noise);
			Voice.Crackle.Step(Noise * Voice.Env);
			Voice.Env *= Voice.Decay;
			const float Roar = Voice.Roar.Low * (Voice.bTorch ? TorchRoar : CampfireRoar);
			const float Value = (Roar * Voice.Flicker + Voice.Crackle.Band * 3) * Voice.Gain * Now.Fires;
			L += Value * Voice.Pan.L;
			R += Value * Voice.Pan.R;
		}

		// the splashes
		for (FSplashVoice& Splash : SplashList)
		{
			if (!Splash.bOn)
			{
				continue;
			}
			const float T = Splash.Time;
			Splash.Time += Dt;
			float Value = 0;
			const float Noise = SplashRandom.Signed();
			if (Splash.Kind != ESplash::Boil)
			{
				const bool bDunk = Splash.Kind == ESplash::Dunk;
				const float Attack = 1 - FMath::Exp(-T * Slap);
				Splash.Slap.Step(Noise);
				Splash.Wash.Step(Noise);
				const float From = bDunk ? 150.f : 240.f, To = bDunk ? 50.f : 90.f;
				Splash.Phase += (To + (From - To) * FMath::Exp(-T / 0.05f)) * Dt;
				Wrap(Splash.Phase);
				Value += Splash.Slap.Low * Attack * FMath::Exp(-T * (bDunk ? DunkSlap : Plunge));
				Value += Splash.Wash.Low * 0.35f * (1 - FMath::Exp(-T / 0.04f)) * FMath::Exp(-T / (bDunk ? 0.55f : 0.3f));
				Value += Sine(Splash.Phase) * 0.6f * Attack * FMath::Exp(-T / 0.06f);
			}
			else
			{
				Splash.Wash.Step(Noise);
				Value += Splash.Wash.Low * 0.15f * FMath::Exp(-T / 0.4f);
			}
			for (int32 Index = 0; Index < FSplashVoice::BubbleCount; ++Index)
			{
				const float Age = T - Splash.BubbleAt[Index];
				if (Age >= 0 && Age < 0.03f)
				{
					Splash.BubblePhase[Index] += Splash.BubbleHz[Index] * (1 + 2 * Age / 0.03f) * Dt;
					Wrap(Splash.BubblePhase[Index]);
					Value += Sine(Splash.BubblePhase[Index]) * 0.3f * FMath::Exp(-Age / 0.012f);
				}
			}
			Value *= Splash.Size;
			L += Value * Splash.Pan.L;
			R += Value * Splash.Pan.R;
		}

		// the lianas
		if (RustleNoise > 1e-6f || RustleFlick > 1e-4f)
		{
			const float Flick = RustleRandom.Signed() * RustleFlick * 0.15f;
			RustleFlick *= ClickFade;
			RustleL.Step(RustleRandom.Signed() * RustleNoise + Flick);
			RustleR.Step(RustleRandom.Signed() * RustleNoise + Flick);
			L += RustleL.Band * RustlePan.L;
			R += RustleR.Band * RustlePan.R;
		}

		Left[Frame] = L * Heard * Master;
		Right[Frame] = R * Heard * Master;
	}
}
