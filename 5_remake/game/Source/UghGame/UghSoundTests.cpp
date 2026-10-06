// The sounds as automation tests of the editor (Ugh.Sounds): every event of the logic with a sound of the original
// has its file, the mixer and when the game plays what.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghSoundPlayer.h"

namespace
{
	/** The events without a sound in the original (the port plays none there). */
	const int32 SilentEvents[] = {
		UGH_LOGIC_EVENT_COPTER_CRASHED, UGH_LOGIC_EVENT_LEVEL_DONE, UGH_LOGIC_EVENT_PASSENGER_BOARDED,
		UGH_LOGIC_EVENT_PASSENGER_PAID, UGH_LOGIC_EVENT_PASSENGER_IN_WATER, UGH_LOGIC_EVENT_ENEMY_STUNNED,
		UGH_LOGIC_EVENT_BONUS_COLLECTED };

	FString SoundDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets/sound"));
	}

	ugh_logic_event Event(int32 Kind, int32 Entity = -1)
	{
		return ugh_logic_event{ Kind, -1, Entity, 0 };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghSoundFilesTest, "Ugh.Sounds.Files",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Every event has a sound or had none in the original; every sound's file is there (.\gradlew.bat :extractor:sound). */
bool FUghSoundFilesTest::RunTest(const FString& Parameters)
{
	for (int32 Kind = UGH_LOGIC_EVENT_LEVEL_CAPTION; Kind <= UGH_LOGIC_EVENT_BONUS_COLLECTED; ++Kind)
	{
		const bool bSilent = TArrayView<const int32>(SilentEvents).Contains(Kind);
		TestTrue(FString::Printf(TEXT("event %d has a sound or is silent"), Kind),
			(FUghSounds::CueOf(Kind) != nullptr) != bSilent);
	}
	int32 Rate = 0;
	for (const TCHAR* Name : FUghSounds::Names())
	{
		TArray<int16> Samples;
		FString Error;
		const bool bRead = FUghSounds::ReadWav(SoundDir() / FString(Name) + TEXT(".wav"), Rate, Samples, Rate, Error);
		TestTrue(Error, bRead && Samples.Num() > Rate / 10);
	}
	TestEqual(TEXT("the rate of the port's synthesizer"), Rate, 49716);
	FUghSounds Sounds;
	Sounds.Load(SoundDir());
	for (const FUghSounds::FCue& Cue : FUghSounds::Cues())
	{
		TestNotNull(FString::Printf(TEXT("the sound of event %d"), Cue.Event), Sounds.Find(Cue.Name));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghMixerTest, "Ugh.Sounds.Mixer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghMixerTest::RunTest(const FString& Parameters)
{
	const TArray<int16> One = { 1000, 1000 }, Loop = { 10, 20, 30 }, Music = { 5 };
	TArray<int16> Out;
	Out.SetNumZeroed(4);
	FUghMixer Mixer;
	Mixer.PlayEffect(One);
	Mixer.PlayEffect(Loop, 7, true);
	Mixer.Mix(Out);
	TestEqual(TEXT("both effects"), Out[0], int16(1010));
	TestEqual(TEXT("the loop alone, from its start again"), Out[3], int16(10));
	TestEqual(TEXT("the effect played once is over"), Mixer.EffectCount(), 1);
	Mixer.StopLoop(Loop, 6);
	TestEqual(TEXT("another owner's loop goes on"), Mixer.EffectCount(), 1);
	Mixer.StopLoop(Loop, 7);
	TestEqual(TEXT("the owner stops its loop"), Mixer.EffectCount(), 0);
	for (int32 Effect = 0; Effect <= FUghMixer::Channels; ++Effect)
	{
		Mixer.PlayEffect(Effect == 0 ? Loop : One, INDEX_NONE, Effect == 0);
	}
	TestEqual(TEXT("a fifth effect takes the place of the first"), Mixer.EffectCount(), FUghMixer::Channels);
	Mixer.StopEffects();
	Mixer.PlayMusic(Music, 2);
	Mixer.Mix(Out);
	TestTrue(TEXT("the music waits, then repeats"), Out[1] == 0 && Out[2] == 5 && Out[3] == 5);
	Mixer.SetVolumes(0.5f, 1.f);
	Mixer.FadeOutMusic(2);
	Mixer.Mix(Out);
	TestTrue(TEXT("half as loud, fading, then over"), Out[0] == 2 && Out[1] == 1 && Out[2] == 0);
	TestNull(TEXT("no music after its fade"), Mixer.GetMusic());
	Mixer.SetVolumes(1.f, 0.25f);
	Mixer.PlayEffect(One);
	Mixer.Mix(Out);
	TestEqual(TEXT("the effects a quarter as loud"), Out[0], int16(250));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghSoundPlayerTest, "Ugh.Sounds.Player",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** When the game plays what: the menu, the play of a level, its end, a lost game. */
bool FUghSoundPlayerTest::RunTest(const FString& Parameters)
{
	FUghSoundPlayer Player;
	Player.Load(SoundDir());
	const FUghSounds& Sounds = Player.GetSounds();
	FUghMixer& Mixer = Player.GetMixer();
	if (!TestNotNull(TEXT("the sounds (.\\gradlew.bat :extractor:sound)"), Sounds.Find(FUghSounds::MenuMusic)))
	{
		return false;
	}
	TestEqual(TEXT("the menu's music"), Mixer.GetMusic(), Sounds.Find(FUghSounds::MenuMusic));
	Player.OnNewGame();
	Player.OnEvent(Event(UGH_LOGIC_EVENT_LEVEL_CAPTION));
	TestEqual(TEXT("the caption's jingle"), Mixer.EffectCount(), 1);
	ugh_logic_view View{};
	View.phase = UGH_LOGIC_PHASE_PLAY;
	Player.OnView(View);
	TestEqual(TEXT("the level's music"), Mixer.GetMusic(), Sounds.Find(FUghSounds::GameMusic));
	Player.OnEvent(Event(UGH_LOGIC_EVENT_FLYER_FLAP_START, 3));
	Player.OnEvent(Event(UGH_LOGIC_EVENT_PASSENGER_PAID));
	TestEqual(TEXT("the flapping, a paid fare is silent"), Mixer.EffectCount(), 2);
	Player.OnEvent(Event(UGH_LOGIC_EVENT_FLYER_FLAP_STOP, 3));
	TestEqual(TEXT("the flapping stops"), Mixer.EffectCount(), 1);
	View.phase = UGH_LOGIC_PHASE_CAPTION;
	Player.OnView(View);
	TestEqual(TEXT("the end of the play stops the effects"), Mixer.EffectCount(), 0);
	Player.OnGameEnd(UGH_LOGIC_GAME_OVER);
	TestEqual(TEXT("a lost game's jingle"), Mixer.EffectCount(), 1);
	TestEqual(TEXT("then the menu's music"), Mixer.GetMusic(), Sounds.Find(FUghSounds::MenuMusic));
	Player.OnGameEnd(UGH_LOGIC_ALL_LEVELS_DONE);
	TestEqual(TEXT("the ending's music"), Mixer.GetMusic(), Sounds.Find(FUghSounds::EndingMusic));
	Player.SetVolumes(70, 50, 100);
	TestTrue(TEXT("the volumes: all of the music's and the effects'"),
		FMath::IsNearlyEqual(Mixer.GetMusicVolume(), 0.35f) && FMath::IsNearlyEqual(Mixer.GetEffectsVolume(), 0.7f));
	return true;
}

#endif
