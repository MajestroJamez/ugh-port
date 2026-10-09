// The ambience as automation tests of the editor (Ugh.Sounds.Ambience): never heard - its samples analysed, and
// written as WAV files (Saved\Ambience) to be listened to later.
#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UghShapes.h"
#include "UghSoundPlayer.h"
#include "UghStage.h"

namespace
{
	using EKind = FUghMood::EKind;
	using EPlace = FUghAmbience::EPlace;
	using ESplash = FUghAmbience::ESplash;
	using ESource = FUghAmbience::ESource;

	constexpr int32 Rate = 49716;

	struct FStereo
	{
		TArray<float> L, R;
		int32 Num() const { return L.Num(); }
		void Append(const FStereo& Other) { L.Append(Other.L); R.Append(Other.R); }
	};

	FStereo Render(FUghAmbience& Ambience, double Seconds)
	{
		FStereo Out;
		Out.L.SetNumZeroed(FMath::RoundToInt(Seconds * Rate));
		Out.R.SetNumZeroed(Out.L.Num());
		Ambience.Render(Out.L, Out.R);
		return Out;
	}

	double Rms(TConstArrayView<float> Samples)
	{
		double Sum = 0;
		for (const float Sample : Samples)
		{
			Sum += double(Sample) * Sample;
		}
		return Samples.IsEmpty() ? 0 : FMath::Sqrt(Sum / Samples.Num());
	}

	double Rms(const FStereo& Sound)
	{
		return FMath::Sqrt((FMath::Square(Rms(Sound.L)) + FMath::Square(Rms(Sound.R))) / 2);
	}

	double Peak(const FStereo& Sound)
	{
		double Most = 0;
		for (int32 Index = 0; Index < Sound.Num(); ++Index)
		{
			Most = FMath::Max(Most, FMath::Max(FMath::Abs(Sound.L[Index]), FMath::Abs(Sound.R[Index])));
		}
		return Most;
	}

	bool Finite(const FStereo& Sound)
	{
		for (int32 Index = 0; Index < Sound.Num(); ++Index)
		{
			if (!FMath::IsFinite(Sound.L[Index]) || !FMath::IsFinite(Sound.R[Index]))
			{
				return false;
			}
		}
		return true;
	}

	/** The RMS of both channels between `Low` and `High` Hz (two band passes in a row). */
	double Band(const FStereo& Sound, float Low, float High)
	{
		const float Middle = FMath::Sqrt(Low * High), Q = Middle / (High - Low);
		double Sum = 0;
		for (const TArray<float>* Channel : { &Sound.L, &Sound.R })
		{
			FUghAmbience::FFilter First, Second;
			First.Set(Middle, Q, Rate);
			Second.Set(Middle, Q, Rate);
			for (const float Sample : *Channel)
			{
				First.Step(Sample);
				Second.Step(First.Band);
				Sum += double(Second.Band) * Second.Band;
			}
		}
		return FMath::Sqrt(Sum / FMath::Max(1, 2 * Sound.Num()));
	}

	FStereo Minus(const FStereo& A, const FStereo& B)
	{
		FStereo Out = A;
		for (int32 Index = 0; Index < Out.Num(); ++Index)
		{
			Out.L[Index] -= B.L[Index];
			Out.R[Index] -= B.R[Index];
		}
		return Out;
	}

	FStereo Slice(const FStereo& Sound, double From, double To)
	{
		const int32 Start = FMath::Clamp(FMath::RoundToInt(From * Rate), 0, Sound.Num());
		const int32 End = FMath::Clamp(FMath::RoundToInt(To * Rate), Start, Sound.Num());
		FStereo Out;
		Out.L = TArray<float>(Sound.L.GetData() + Start, End - Start);
		Out.R = TArray<float>(Sound.R.GetData() + Start, End - Start);
		return Out;
	}

	/** Writes `Sound` as a 16-bit stereo WAV file into Saved\Ambience (to be listened to, not by the tests). */
	void Write(const FString& Name, const FStereo& Sound)
	{
		TArray<uint8> Bytes;
		auto Add32 = [&Bytes](uint32 Value) { Bytes.Append(reinterpret_cast<const uint8*>(&Value), 4); };
		auto Add16 = [&Bytes](uint16 Value) { Bytes.Append(reinterpret_cast<const uint8*>(&Value), 2); };
		auto Tag = [&Bytes](const char* Text) { Bytes.Append(reinterpret_cast<const uint8*>(Text), 4); };
		const uint32 Data = Sound.Num() * 4;
		Tag("RIFF"); Add32(36 + Data); Tag("WAVE");
		Tag("fmt "); Add32(16); Add16(1); Add16(2); Add32(Rate); Add32(Rate * 4); Add16(4); Add16(16);
		Tag("data"); Add32(Data);
		for (int32 Index = 0; Index < Sound.Num(); ++Index)
		{
			Add16(uint16(int16(FMath::Clamp(Sound.L[Index] * 32767.f, -32768.f, 32767.f))));
			Add16(uint16(int16(FMath::Clamp(Sound.R[Index] * 32767.f, -32768.f, 32767.f))));
		}
		FFileHelper::SaveArrayToFile(Bytes, *(FPaths::ProjectSavedDir() / TEXT("Ambience") / Name + TEXT(".wav")));
	}

	/** A level's fires as the play's camera sees them: two campfires, three torches, a campfire under the water. */
	TArray<FUghAmbience::FFire> LevelFires()
	{
		return {
			{ UghShapes::ToWorld(60, 150, 0), ESource::Campfire, true },
			{ UghShapes::ToWorld(250, 80, 0), ESource::Campfire, true },
			{ UghShapes::ToWorld(120, 60, 20), ESource::Torch, true },
			{ UghShapes::ToWorld(200, 120, 20), ESource::Torch, true },
			{ UghShapes::ToWorld(300, 40, 20), ESource::Torch, true },
			{ UghShapes::ToWorld(160, 180, 0), ESource::Campfire, false } };
	}

	/** An ambience heard from the play's camera, `Mood`, at `Place`, fully seen, settled for `Settle` seconds. */
	void Start(FUghAmbience& Ambience, EKind Mood, EPlace Place, TConstArrayView<FUghAmbience::FFire> Fires,
		double Settle = 4)
	{
		const FUghCameraPose Camera = AUghStage::Play(16.0 / 9);
		Ambience.SetListener(Camera.Location, Camera.Rotation);
		Ambience.SetFires(Fires);
		Ambience.SetScene(Mood, Place, 1);
		Render(Ambience, Settle);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghAmbienceMoodsTest, "Ugh.Sounds.Ambience.Moods",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * Every mood in the play (and the other places): heard, never clipping, as loud as it should be beside the music; the
 * storm louder, with its wind and rain; the birds by day, the crickets at night. Written to Saved\Ambience.
 */
bool FUghAmbienceMoodsTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* Name;
		EKind Mood;
		EPlace Place;
	};
	const FCase Cases[] = { { TEXT("play-day"), EKind::Day, EPlace::Play }, { TEXT("play-evening"), EKind::Evening, EPlace::Play },
		{ TEXT("play-dusk"), EKind::Dusk, EPlace::Play }, { TEXT("play-night"), EKind::Night, EPlace::Play },
		{ TEXT("play-storm"), EKind::Storm, EPlace::Play }, { TEXT("menu-day"), EKind::Day, EPlace::Menu },
		{ TEXT("isles-evening"), EKind::Evening, EPlace::Isles }, { TEXT("flight-night"), EKind::Night, EPlace::Flight },
		{ TEXT("flight-storm"), EKind::Storm, EPlace::Flight } };
	const TArray<FUghAmbience::FFire> Fires = LevelFires();
	TMap<FString, FStereo> Heard;
	for (const FCase& Case : Cases)
	{
		FUghAmbience Ambience(Rate);
		Start(Ambience, Case.Mood, Case.Place, Fires);
		const FStereo Sound = Render(Ambience, 20);
		Write(Case.Name, Sound);
		const double Loud = Rms(Sound), Top = Peak(Sound);
		AddInfo(FString::Printf(TEXT("%s: RMS %.4f (%.1f dBFS), peak %.3f, birds (1.9-4 kHz) %.5f, crickets (4.3-5.3 kHz) %.5f, rain (8-12 kHz) %.5f, fires %d"),
			Case.Name, Loud, 20 * FMath::LogX(10.0, Loud), Top, Band(Sound, 1900, 4000), Band(Sound, 4300, 5300),
			Band(Sound, 8000, 12000), Ambience.FireVoicesSounding()));
		TestTrue(FString(Case.Name) + TEXT(": numbers"), Finite(Sound));
		TestTrue(FString(Case.Name) + TEXT(": heard, under the music (-44 .. -27 dBFS)"), Loud > 0.0063 && Loud < 0.045);
		TestTrue(FString(Case.Name) + TEXT(": never clipping"), Top < 0.9);
		Heard.Add(Case.Name, Sound);
	}
	const FStereo& Day = Heard[TEXT("play-day")];
	const FStereo& Night = Heard[TEXT("play-night")];
	const FStereo& Storm = Heard[TEXT("play-storm")];
	TestTrue(TEXT("the storm louder than the day"), Rms(Storm) > 1.5 * Rms(Day));
	TestTrue(TEXT("the storm's rain hisses"), Band(Storm, 8000, 12000) > 5 * Band(Day, 8000, 12000));
	// the jungle alone (the fires crackle at every pitch)
	auto Jungle = [](EKind Mood)
	{
		FUghAmbience Ambience(Rate);
		Start(Ambience, Mood, EPlace::Play, {});
		return Render(Ambience, 20);
	};
	const FStereo DayJungle = Jungle(EKind::Day), NightJungle = Jungle(EKind::Night);
	AddInfo(FString::Printf(TEXT("without fires: by day birds %.5f, crickets %.5f; at night birds %.5f, crickets %.5f"),
		Band(DayJungle, 1900, 4000), Band(DayJungle, 4300, 5300), Band(NightJungle, 1900, 4000), Band(NightJungle, 4300, 5300)));
	TestTrue(TEXT("crickets at night, not by day"), Band(NightJungle, 4300, 5300) > 3 * Band(DayJungle, 4300, 5300));
	TestTrue(TEXT("birds by day, not at night"), Band(DayJungle, 1900, 4000) > 1.5 * Band(NightJungle, 1900, 4000));

	// beside the music: the ambience at its full volume well under it
	FUghSounds Sounds;
	Sounds.Load(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets/sound")));
	if (const TArray<int16>* Music = Sounds.Find(FUghSounds::GameMusic))
	{
		double Sum = 0;
		for (const int16 Sample : *Music)
		{
			Sum += double(Sample) * Sample;
		}
		const double MusicRms = FMath::Sqrt(Sum / FMath::Max(1, Music->Num())) / 32768;
		AddInfo(FString::Printf(TEXT("the level's music: RMS %.4f (%.1f dBFS)"), MusicRms, 20 * FMath::LogX(10.0, MusicRms)));
		TestTrue(TEXT("the day's ambience well under the music, the storm's under it"),
			Rms(Day) < 0.65 * MusicRms && Rms(Storm) < MusicRms);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghAmbienceFiresTest, "Ugh.Sounds.Ambience.Fires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The fires where they are: a fire on the left heard on the left, the nearer louder, at most FireVoices of them (the
 * loudest), one under the water silent; the ears in front of the play.
 */
bool FUghAmbienceFiresTest::RunTest(const FString& Parameters)
{
	const FUghCameraPose Camera = AUghStage::Play(16.0 / 9);
	FUghAmbience Probe(Rate);
	Probe.SetListener(Camera.Location, Camera.Rotation);
	const FVector Ears = Probe.Ears();
	AddInfo(FString::Printf(TEXT("the play's camera %.0f units from the plane, the ears %.0f"), FMath::Abs(Camera.Location.Y),
		FMath::Abs(Ears.Y)));
	TestTrue(TEXT("the ears EarsFromPlane in front of the play (on the line of sight)"),
		FMath::Abs(Ears.Y) > 0.85 * FUghAmbience::EarsFromPlane && FMath::Abs(Ears.Y) < FUghAmbience::EarsFromPlane + 1);
	float Middle = 0, Edge = 0, Pan = 0, LeftPan = 0;
	Probe.Locate(UghShapes::ToWorld(160, 96, 0), FUghAmbience::FireNear, Middle, Pan);
	Probe.Locate(UghShapes::ToWorld(10, 96, 0), FUghAmbience::FireNear, Edge, LeftPan);
	TestTrue(TEXT("a fire in the middle louder than one at the edge"), Middle > 1.2f * Edge && Edge > 0.1f);
	TestTrue(TEXT("one on the left on the left"), LeftPan < -0.5f && FMath::Abs(Pan) < 0.1f);
	float Near = 0;
	FUghAmbience Close(Rate);
	Close.SetListener(UghShapes::ToWorld(160, 96, -600), Camera.Rotation);
	Close.Locate(UghShapes::ToWorld(160, 96, 0), FUghAmbience::FireNear, Near, Pan);
	TestTrue(TEXT("nearer (a close-up) louder"), Near > Middle);

	// the fire's own sound: the same ambience with it less the one without it
	auto Heard = [](TConstArrayView<FUghAmbience::FFire> Fires)
	{
		FUghAmbience Ambience(Rate);
		Start(Ambience, EKind::Night, EPlace::Play, Fires, 1);
		return Render(Ambience, 6);
	};
	const FStereo Without = Heard({});
	const FUghAmbience::FFire Left{ UghShapes::ToWorld(30, 150, 0), ESource::Campfire, true };
	const FStereo Fire = Minus(Heard({ Left }), Without);
	Write(TEXT("campfire-left"), Fire);
	AddInfo(FString::Printf(TEXT("a campfire on the left: RMS left %.5f, right %.5f, peak %.3f"), Rms(Fire.L), Rms(Fire.R),
		Peak(Fire)));
	TestTrue(TEXT("a campfire heard"), Rms(Fire) > 0.001);
	TestTrue(TEXT("on the left"), Rms(Fire.L) > 2 * Rms(Fire.R));
	const FStereo Torch = Minus(Heard({ { Left.Place, ESource::Torch, true } }), Without);
	Write(TEXT("torch-left"), Torch);
	TestTrue(TEXT("a torch quieter than a campfire"), Rms(Torch) > 0.0005 && Rms(Torch) < Rms(Fire));
	TestTrue(TEXT("a campfire under the water silent"), Rms(Minus(Heard({ { Left.Place, ESource::Campfire, false } }), Without)) < 1e-6);

	// many fires: the loudest FireVoices heard
	TArray<FUghAmbience::FFire> Many;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		Many.Add({ UghShapes::ToWorld(20 + 30 * Index, 100, Index == 4 ? 0 : 800), ESource::Campfire, true });
	}
	FUghAmbience Ambience(Rate);
	Start(Ambience, EKind::Night, EPlace::Play, Many, 1);
	TestEqual(TEXT("at most FireVoices fires heard"), Ambience.FireVoicesSounding(), FUghAmbience::FireVoices);
	TestTrue(TEXT("the nearest among them"), Ambience.IsFireHeard(4));
	TestFalse(TEXT("not the farthest on the left"), Ambience.IsFireHeard(0));
	TestFalse(TEXT("not the farthest on the right"), Ambience.IsFireHeard(9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghAmbienceEventsTest, "Ugh.Sounds.Ambience.Events",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The splashes (a passenger flung into the sea, a copter falling in, its foam), the lianas rustling, the fade with the
 * picture, the volume, the same sound for the same scene, the cost.
 */
bool FUghAmbienceEventsTest::RunTest(const FString& Parameters)
{
	const FVector Sea = UghShapes::ToWorld(160, 170, 0);
	auto With = [&](TFunction<void(FUghAmbience&)> Act, double Seconds)
	{
		FUghAmbience Ambience(Rate);
		Start(Ambience, EKind::Day, EPlace::Play, {}, 1);
		Act(Ambience);
		return Render(Ambience, Seconds);
	};
	const FStereo Still = With([](FUghAmbience&) {}, 3);
	FStereo Splashes;
	double Loud[3] = {};
	for (const ESplash Kind : { ESplash::Plunge, ESplash::Dunk, ESplash::Boil })
	{
		int32 Sounding = 0;
		const FStereo Splash = Minus(With([&](FUghAmbience& Ambience)
		{
			Ambience.Splash(Kind, Sea, 1);
			Sounding = Ambience.SplashesSounding();
		}, 3), Still);
		Splashes.Append(Splash);
		Loud[static_cast<int32>(Kind)] = Rms(Slice(Splash, 0, 0.5));
		TestEqual(TEXT("a splash sounds"), Sounding, 1);
		TestTrue(TEXT("a splash over within 2.5 s"), Rms(Slice(Splash, 2.5, 3)) < 1e-6 && Peak(Splash) < 0.9);
	}
	Write(TEXT("splashes-plunge-dunk-boil"), Splashes);
	const double Plunge = Loud[0], Dunk = Loud[1], Boil = Loud[2];   // (ESplash)
	AddInfo(FString::Printf(TEXT("splashes: plunge %.4f, dunk %.4f, boil %.4f (RMS of the first half second)"),
		Plunge, Dunk, Boil));
	TestTrue(TEXT("a copter falling in louder than a passenger"), Dunk > Plunge);
	TestTrue(TEXT("its foam softest"), Boil > 0.001 && Boil < Plunge);

	const FStereo Rustle = Minus(With([](FUghAmbience& Ambience)
	{
		Ambience.SetRustle(1, UghShapes::ToWorld(20, 60, 0));
	}, 3), Still);
	Write(TEXT("rustle-left"), Rustle);
	AddInfo(FString::Printf(TEXT("the lianas: RMS %.4f, left %.4f, right %.4f"), Rms(Rustle), Rms(Rustle.L), Rms(Rustle.R)));
	TestTrue(TEXT("the lianas rustle on the left"), Rms(Rustle) > 0.002 && Rms(Rustle.L) > 1.5 * Rms(Rustle.R));
	TestTrue(TEXT("still lianas silent"), Rms(Minus(With([](FUghAmbience& Ambience)
	{
		Ambience.SetRustle(0, FVector::ZeroVector);
	}, 3), Still)) < 1e-6);

	// heard as much as seen: in the black silent, fading in smoothly
	FUghAmbience Fading(Rate);
	Start(Fading, EKind::Day, EPlace::Play, {}, 4);
	Fading.SetScene(EKind::Day, EPlace::Play, 0);
	Render(Fading, 4);
	TestTrue(TEXT("silent in the black"), Rms(Render(Fading, 1)) < 1e-4);
	Fading.SetScene(EKind::Day, EPlace::Play, 1);
	const FStereo In = Render(Fading, 2);
	const double First = Rms(Slice(In, 0, 0.1)), Later = Rms(Slice(In, 1.5, 2));
	AddInfo(FString::Printf(TEXT("the fade in: %.5f then %.5f"), First, Later));
	TestTrue(TEXT("fading in, not at once"), First < 0.4 * Later && Later > 0.005);

	// the volume, both channels around the mixer's middle
	FUghSoundPlayer Player;
	Player.GetAmbience().SetScene(EKind::Storm, EPlace::Play, 1);
	TArray<int16> Out;
	Out.SetNumZeroed(2 * Rate);
	Player.MixStereo(Out);
	Player.MixStereo(Out);
	int32 Most = 0;
	for (const int16 Sample : Out)
	{
		Most = FMath::Max(Most, FMath::Abs(int32(Sample)));
	}
	TestTrue(TEXT("the ambience in the stream"), Most > 100);
	Player.SetVolumes(100, 100, 100, 0);
	Player.MixStereo(Out);
	TestTrue(TEXT("its volume 0: silent"), !Out.ContainsByPredicate([](int16 Sample) { return Sample != 0; }));
	Player.SetVolumes(50, 100, 100, 50);
	TestTrue(TEXT("the ambience's volume: all of it times its own"), FMath::IsNearlyEqual(Player.GetAmbienceVolume(), 0.25f));

	// deterministic; cheap
	auto Busy = []()
	{
		FUghAmbience Ambience(Rate);
		TArray<FUghAmbience::FFire> Fires = LevelFires();
		Fires.Append(LevelFires());
		Start(Ambience, EKind::Storm, EPlace::Play, Fires, 0.5);
		Ambience.SetRustle(0.8f, UghShapes::ToWorld(300, 50, 0));
		for (const ESplash Kind : { ESplash::Plunge, ESplash::Dunk, ESplash::Boil, ESplash::Dunk })
		{
			Ambience.Splash(Kind, UghShapes::ToWorld(100, 170, 0), 1.5);
		}
		const double Started = FPlatformTime::Seconds();
		FStereo Sound = Render(Ambience, 10);
		return TPair<FStereo, double>(MoveTemp(Sound), FPlatformTime::Seconds() - Started);
	};
	const TPair<FStereo, double> One = Busy(), Two = Busy();
	TestTrue(TEXT("the same scene, the same sound"), One.Key.L == Two.Key.L && One.Key.R == Two.Key.R);
	AddInfo(FString::Printf(TEXT("10 s of a storm with %d fires, the lianas and 4 splashes made in %.0f ms"),
		FUghAmbience::FireVoices, 1000 * FMath::Min(One.Value, Two.Value)));
	TestTrue(TEXT("made in under 3 % of the time it plays"), FMath::Min(One.Value, Two.Value) < 0.3);
	return true;
}

#endif
