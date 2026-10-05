// The bursts of the events as automation tests of the editor: every event of the logic has a burst (Ugh.Effects.Cues),
// an event plays its sound and shows its burst from the same event, where it happens (Ugh.Effects.Events), a copter
// landing on a pad and a bonus item landing raise dust (Ugh.Effects.Landing).
#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/AllOf.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghEffectPlayer.h"
#include "UghEvents.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghSoundPlayer.h"

namespace
{
	/** A burst played once is over within this many seconds (the surf of the caption the longest). */
	constexpr double LongestBurst = 6;
	/** Dust, smoke and spray never cover more than this much of what is behind them (a figure stays seen). */
	constexpr float MostCover = 0.7f;
	/** The view of the tests: a copter, a passenger, an enemy and a bonus item (pixels), the water. */
	constexpr int32 CopterX = 100, CopterY = 60, WaterRow = 170;
	constexpr int32 Passenger = 2, Enemy = 1, Item = 0;
	const FVector2D EnemyAt(200, 50), ItemAt(150, 120);
	/** The sprites without their sizes (UghSprites' default). */
	constexpr double Sprite = 16;

	int32 Subpixels(double Pixels) { return FMath::RoundToInt32(Pixels * UghShapes::Subpixels); }

	ugh_logic_view PlayView()
	{
		ugh_logic_view View{};
		View.phase = UGH_LOGIC_PHASE_PLAY;
		View.fade = UghShapes::FadeShown;
		View.copter_count = 2;
		View.copters[0] = { Subpixels(CopterX), Subpixels(CopterY), 0, 0, 0, 0 };
		View.copters[1] = { Subpixels(CopterX + 80), Subpixels(CopterY), 0, 0, 0, 0 };
		View.water_level = Subpixels(WaterRow);
		View.entity_count = 3;
		View.entities[0] = { UGH_LOGIC_ENTITY_PASSENGER, Passenger, Subpixels(60), Subpixels(100), 0, -1, 1, 0 };
		View.entities[1] = { UGH_LOGIC_ENTITY_ENEMY, Enemy, Subpixels(EnemyAt.X), Subpixels(EnemyAt.Y), 0, -1, 0, 0 };
		View.entities[2] = { UGH_LOGIC_ENTITY_BONUS_ITEM, Item, Subpixels(ItemAt.X), Subpixels(ItemAt.Y), 0, -1, 0, 0 };
		return View;
	}

	/** The event of `Kind` as the logic reports it: its player, entity and value. */
	ugh_logic_event EventOf(int32 Kind)
	{
		switch (Kind)
		{
		case UGH_LOGIC_EVENT_LEVEL_CAPTION: case UGH_LOGIC_EVENT_LEVEL_DONE: return { Kind, -1, -1, 0 };
		case UGH_LOGIC_EVENT_COPTER_CRASHED: return { Kind, 0, -1, 0 };
		case UGH_LOGIC_EVENT_PASSENGER_PAID: return { Kind, 0, Passenger, 150 };
		case UGH_LOGIC_EVENT_PASSENGER_IN_WATER: return { Kind, -1, Passenger, 0 };
		case UGH_LOGIC_EVENT_BONUS_COLLECTED: return { Kind, 0, Item, 1 };
		case UGH_LOGIC_EVENT_PASSENGER_BOARDED: case UGH_LOGIC_EVENT_QUICK_DELIVERY:
		case UGH_LOGIC_EVENT_PASSENGER_DROPPED: return { Kind, 0, Passenger, 0 };
		case UGH_LOGIC_EVENT_ENEMY_STUNNED: return { Kind, -1, Enemy, 300 };
		default: return { Kind, -1, Enemy, 0 };
		}
	}

	FString SoundDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets/sound"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghEffectCuesTest, "Ugh.Effects.Cues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Every event has a burst, a loop of the sound a loop of the burst; every burst is short and named for the shots. */
bool FUghEffectCuesTest::RunTest(const FString& Parameters)
{
	for (int32 Kind = UGH_LOGIC_EVENT_LEVEL_CAPTION; Kind <= UGH_LOGIC_EVENT_BONUS_COLLECTED; ++Kind)
	{
		const FUghEffectCue* Cue = FUghEffectPlayer::CueOf(Kind);
		if (!TestNotNull(FString::Printf(TEXT("the burst of event %d"), Kind), Cue))
		{
			continue;
		}
		const FUghSounds::FCue* Sound = FUghSounds::CueOf(Kind);
		const bool bSoundLoops = Sound && Sound->Action != FUghSounds::EAction::Play;
		TestEqual(FString::Printf(TEXT("event %d loops (or stops) its sound and its burst alike"), Kind),
			Cue->Action != EUghEffectAction::Play, bSoundLoops);
		TestEqual(FString::Printf(TEXT("event %d loops a repeating burst"), Kind), Cue->Action != EUghEffectAction::Play,
			UghBursts::Get(Cue->Burst).bRepeat);
	}
	for (int32 Index = 0; Index < int32(EUghBurst::Count); ++Index)
	{
		const EUghBurst Burst = EUghBurst(Index);
		const UghBursts::FBurst& Each = UghBursts::Get(Burst);
		TestEqual(FString::Printf(TEXT("burst %s found by its name"), Each.Name), UghBursts::Find(Each.Name),
			TOptional<EUghBurst>(Burst));
		TestTrue(FString::Printf(TEXT("burst %s has particles"), Each.Name), !Each.Parts.IsEmpty() &&
			Algo::AllOf(Each.Parts, [](const UghBursts::FPart& Part) { return Part.Count > 0 && Part.Life > 0; }));
		TestTrue(FString::Printf(TEXT("burst %s is short"), Each.Name), Each.bRepeat ||
			UghBursts::Lasts(Burst) <= LongestBurst);
		for (const UghBursts::FPart& Part : Each.Parts)
		{
			TestTrue(FString::Printf(TEXT("burst %s lets a figure be seen"), Each.Name),
				Part.Blend != UghBursts::EBlend::Burst || Part.Strength <= MostCover);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghEffectEventsTest, "Ugh.Effects.Events",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** Each event, given to the sounds and the effects together (UghEvents), plays its sound and shows its burst there. */
bool FUghEffectEventsTest::RunTest(const FString& Parameters)
{
	const ugh_logic_view Previous = PlayView(), Current = PlayView();
	for (int32 Kind = UGH_LOGIC_EVENT_LEVEL_CAPTION; Kind <= UGH_LOGIC_EVENT_BONUS_COLLECTED; ++Kind)
	{
		FUghSoundPlayer Sounds;
		Sounds.Load(SoundDir());
		FUghEffectPlayer Effects;
		Effects.Load(nullptr, nullptr, nullptr);
		const ugh_logic_event Event = EventOf(Kind);
		const FUghSounds::FCue* Sound = FUghSounds::CueOf(Kind);
		if (Sound && Sound->Action == FUghSounds::EAction::Stop)
		{
			// the loop it stops
			UghEvents::Play({ EventOf(UGH_LOGIC_EVENT_FLYER_FLAP_START) }, Previous, Current, Sounds, Effects);
			Effects.TakeOrders();
		}
		UghEvents::Play({ Event }, Previous, Current, Sounds, Effects);
		const TArray<FUghEffectOrder> Orders = Effects.TakeOrders();
		const FUghEffectCue& Cue = *FUghEffectPlayer::CueOf(Kind);
		const FUghEffectOrder* Order = Orders.FindByPredicate([&](const FUghEffectOrder& Each)
		{
			return Each.Event == Kind && Each.Action == Cue.Action &&
				(Each.Burst == Cue.Burst || (Cue.ForFlyer && Each.Burst == *Cue.ForFlyer));
		});
		TestNotNull(FString::Printf(TEXT("event %d shows its burst"), Kind), Order);
		if (Sound && Sounds.GetSounds().Find(Sound->Name))
		{
			TestEqual(FString::Printf(TEXT("event %d plays its sound (or stops it)"), Kind),
				Sounds.GetMixer().EffectCount(), Sound->Action == FUghSounds::EAction::Stop ? 0 : 1);
		}
		else
		{
			TestEqual(FString::Printf(TEXT("event %d is silent"), Kind), Sounds.GetMixer().EffectCount(), 0);
		}
		if (!Order)
		{
			continue;
		}
		switch (Kind)
		{
		case UGH_LOGIC_EVENT_LEVEL_DONE:
			TestEqual(TEXT("the level done over every copter"), Orders.Num(), 2);
			break;
		case UGH_LOGIC_EVENT_PASSENGER_IN_WATER:
			TestEqual(TEXT("a splash on the water"), Order->Place, FVector2D(60 + Sprite / 2, WaterRow));
			break;
		case UGH_LOGIC_EVENT_PASSENGER_PAID:
			TestEqual(TEXT("the fare's points"), Order->Points, 150);
			TestEqual(TEXT("at the copter"), Order->Place, FVector2D(CopterX + (UghShapes::CopterBodyLeft +
				UghShapes::CopterBodyRight + 1) / 2.0, CopterY + UghShapes::CopterBodyHeight / 2.0));
			break;
		case UGH_LOGIC_EVENT_BLOWER_BLOW:
			TestEqual(TEXT("the gust from the blower's side it faces"), Order->Place,
				FVector2D(EnemyAt.X, EnemyAt.Y + Sprite / 2));
			break;
		case UGH_LOGIC_EVENT_BONUS_COLLECTED:
			TestEqual(TEXT("the glints of a life"), Order->Tint, FUghEffectPlayer::BonusTints[1]);
			TestEqual(TEXT("at the item"), Order->Place, ItemAt + Sprite / 2);
			break;
		case UGH_LOGIC_EVENT_ENEMY_STUNNED:
			TestEqual(TEXT("the enemy's points"), Order->Points, 300);
			break;
		default:
			break;
		}
	}

	// an entity gone from the view (a collected bonus item) is where it was last seen
	FUghEffectPlayer Effects;
	Effects.Load(nullptr, nullptr, nullptr);
	Effects.OnView(Previous, Current);
	ugh_logic_view Gone = Current;
	Gone.entity_count = 2;
	Effects.OnEvent(EventOf(UGH_LOGIC_EVENT_BONUS_COLLECTED), Gone);
	const TArray<FUghEffectOrder> Collected = Effects.TakeOrders();
	TestTrue(TEXT("a bonus item's glints where it was last seen"),
		Collected.Num() == 1 && Collected[0].Place == ItemAt + Sprite / 2);

	// a crash into the water splashes too
	ugh_logic_view Sinking = Current;
	Sinking.copters[0].y = Subpixels(WaterRow - UghShapes::CopterBodyHeight / 2);
	Effects.OnEvent(EventOf(UGH_LOGIC_EVENT_COPTER_CRASHED), Sinking);
	const TArray<FUghEffectOrder> Crash = Effects.TakeOrders();
	TestTrue(TEXT("an explosion and a splash"), Crash.Num() == 2 && Crash[0].Burst == EUghBurst::Explosion &&
		Crash[1].Burst == EUghBurst::Splash && Crash[1].Place.Y == WaterRow);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghEffectLandingTest, "Ugh.Effects.Landing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** No event says so: a copter coming down onto a pad raises dust (not hovering onto it), a bonus item landing too. */
bool FUghEffectLandingTest::RunTest(const FString& Parameters)
{
	FUghSimulation Simulation;
	FString Error;
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error)))
	{
		return false;
	}
	Simulation.Preview(FUghGameChoice{ 1, 1, 0 });
	ugh_logic_pad Pad;
	if (!TestTrue(TEXT("level 1 has a pad"), ugh_logic_get_pad(Simulation.GetLogic(), 0, &Pad) != 0))
	{
		return false;
	}
	// the copter's body over the pad's middle, its bottom `Above` pixels over the pad
	auto At = [&Pad, &Simulation](double Above)
	{
		ugh_logic_view View = Simulation.GetCurrent();
		View.phase = UGH_LOGIC_PHASE_PLAY;
		View.copter_count = 1;
		View.copters[0].x = Subpixels((Pad.left + Pad.right) / 2.0 - (UghShapes::CopterBodyLeft +
			UghShapes::CopterBodyRight) / 2.0);
		View.copters[0].y = Subpixels(Pad.y - UghShapes::CopterBodyHeight - Above);
		View.entity_count = 0;
		return View;
	};
	for (const double Step : { 2.0, 0.1 })
	{
		FUghEffectPlayer Effects;
		Effects.Load(Simulation.GetLogic(), nullptr, nullptr);
		// standing on it at the start (no dust), up, down: the last step in the air `Step` pixels, then on the pad
		constexpr double Up = 10, Low = 4;
		Effects.OnView(At(0), At(0));
		Effects.OnView(At(0), At(Up));
		Effects.OnView(At(Up), At(Low));
		Effects.OnView(At(Low), At(Low - Step));
		Effects.OnView(At(Low - Step), At(0));
		const TArray<FUghEffectOrder> Orders = Effects.TakeOrders();
		if (Step > FUghEffectPlayer::LandingPixelsPerStep)
		{
			TestTrue(TEXT("dust under a copter coming down onto a pad"), Orders.Num() == 1 &&
				Orders[0].Burst == EUghBurst::Dust && Orders[0].Place.Y == Pad.y);
		}
		else
		{
			TestEqual(TEXT("no dust under a copter settling onto a pad"), Orders.Num(), 0);
		}
	}
	// a bonus item falling, then lying
	FUghEffectPlayer Effects;
	Effects.Load(Simulation.GetLogic(), nullptr, nullptr);
	ugh_logic_view Falling[3] = { At(0), At(0), At(0) };
	for (int32 Step = 0; Step < 3; ++Step)
	{
		Falling[Step].entity_count = 1;
		Falling[Step].entities[0] = { UGH_LOGIC_ENTITY_BONUS_ITEM, Item, Subpixels(ItemAt.X),
			Subpixels(ItemAt.Y + FMath::Min(Step, 1) * 3), 0, -1, 0, 0 };
	}
	Effects.OnView(Falling[0], Falling[1]);
	Effects.OnView(Falling[1], Falling[2]);
	const TArray<FUghEffectOrder> Landed = Effects.TakeOrders();
	TestTrue(TEXT("a little dust where a bonus item lands"), Landed.Num() == 1 && Landed[0].Burst == EUghBurst::Dust &&
		Landed[0].Place.Y == ItemAt.Y + 3 + Sprite);
	return true;
}

#endif
