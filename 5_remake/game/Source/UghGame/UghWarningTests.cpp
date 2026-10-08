// The warning over a copter flying fast enough to crash, as an automation test of the editor (Ugh.Warning).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghKnockPilot.h"
#include "UghShapes.h"
#include "UghWarning.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	/** The steering key adds this much speed a step (the logic's; the test only uses it to reach a speed). */
	constexpr int32 SteerStep = 63;
	/** The logic plays at most this long (steps) a part of a case. */
	constexpr int32 MostSteps = 3000;
	/** The heights (pixels) of level 1 where the copter flies across from its start into rock: right, left. */
	constexpr int32 RightY = 50, LeftY = 100;
	/** It holds a height by where it will be this many steps on. */
	constexpr int32 LookAhead = 8;

	/** A game of level 1 at a difficulty played step by step with pilot 1's keys. */
	struct FGame
	{
		ugh_logic* Logic = nullptr;
		ugh_logic_view Previous{}, Current{};
		bool Held[5] = {};
		bool bCrashed = false;

		bool Start(int32 Difficulty)
		{
			char Problem[256] = {};
			Logic = ugh_logic_create(TCHAR_TO_UTF8(*(Assets() / TEXT("logic/ugh-data.ugd"))), Problem, sizeof Problem);
			if (!Logic)
			{
				return false;
			}
			ugh_logic_settings Settings;
			ugh_logic_default_settings(&Settings);
			Settings.difficulty = Difficulty;
			ugh_logic_new_game(Logic, &Settings);
			// the caption goes on by a key, the play fades in
			for (int32 Step = 0; Step < MostSteps; ++Step)
			{
				Next();
				if (Current.phase == UGH_LOGIC_PHASE_CAPTION && Step % 20 == 0)
				{
					ugh_logic_menu_key(Logic, UGH_LOGIC_MENU_OTHER);
				}
				if (Current.phase == UGH_LOGIC_PHASE_PLAY && Current.fade >= UGH_LOGIC_FADE_SHOWN)
				{
					return true;
				}
			}
			return false;
		}
		~FGame()
		{
			if (Logic)
			{
				ugh_logic_destroy(Logic);
			}
		}
		void Next()
		{
			Previous = Current;
			ugh_logic_step(Logic);
			ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
			{
				*static_cast<bool*>(Context) |= Event->kind == UGH_LOGIC_EVENT_COPTER_CRASHED;
			}, &bCrashed);
			ugh_logic_get_view(Logic, &Current);
		}
		void Hold(int32 Key, bool bHeld)
		{
			if (Held[Key] != bHeld)
			{
				ugh_logic_key(Logic, 0, Key, bHeld ? 1 : 0);
				Held[Key] = bHeld;
			}
		}
		ugh_logic_copter_danger Danger() const
		{
			ugh_logic_copter_danger Danger{};
			ugh_logic_get_copter_danger(Logic, 0, &Danger);
			return Danger;
		}
		int32 Climb() const { return Current.copters[0].y - Previous.copters[0].y; }
		/** Pedals to hold the height `Y` (pixels). */
		void Keep(int32 Y)
		{
			Hold(UGH_LOGIC_KEY_UP, Current.copters[0].y + LookAhead * Climb() > Y * UghShapes::Subpixels);
		}
		/** Holds the height `Y` until it is there at rest; false when it never is. */
		bool Settle(int32 Y)
		{
			for (int32 Step = 0; Step < MostSteps && !bCrashed; ++Step)
			{
				Keep(Y);
				Next();
				if (FMath::Abs(Current.copters[0].y - Y * UghShapes::Subpixels) <= 2 * UghShapes::Subpixels &&
					FMath::Abs(Climb()) <= UghShapes::Subpixels / 4)
				{
					return true;
				}
			}
			return false;
		}
	};

	/** The fewest steering steps to a speed whose bounce crashes the copter (impact the logic's: rounded to even). */
	int32 StepsToCrash(int32 Limit, int32 Way)
	{
		for (int32 Steps = 1;; ++Steps)
		{
			const int32 Speed = Steps * SteerStep;
			// going right (down) an odd speed is one more, going left (up) one less
			const int32 Impact = Way > 0 ? Speed + (Speed & 1) : Speed - (Speed & 1);
			if (Impact >= Limit)
			{
				return Steps;
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghWarningTest, "Ugh.Warning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The logic of level 1 on each difficulty: copter 0 sped up across to just below and just above the speed that crashes
 * it into the rock ahead (left and right of its start), then coasting into it - the warning shows exactly when the
 * logic crashes it (and its loudness grows with the speed); climbing at full speed into the top of the screen (no
 * crash: no warning) and diving onto the ground (a crash: warned); on hard falling into the open sea (no crash: no
 * warning).
 */
bool FUghWarningTest::RunTest(const FString& Parameters)
{
	const int32 Limits[] = { 3100, 2300, 1380 };   // the original's crash limits, easy .. hard
	for (int32 Difficulty = 0; Difficulty < 3; ++Difficulty)
	{
		for (const int32 Way : { 1, -1 })
		{
			const int32 Crashing = StepsToCrash(Limits[Difficulty], Way);
			for (const int32 Steps : { Crashing - 1, Crashing })
			{
				const FString Case = FString::Printf(TEXT("difficulty %d, %s at %d"), Difficulty,
					Way > 0 ? TEXT("right") : TEXT("left"), Steps * SteerStep);
				FGame Game;
				if (!TestTrue(Case + TEXT(": the play begins"), Game.Start(Difficulty)) ||
					!TestTrue(Case + TEXT(": at its height"), Game.Settle(Way > 0 ? RightY : LeftY)))
				{
					continue;
				}
				TestEqual(Case + TEXT(": the crash limit"), Game.Danger().crash_limit, Limits[Difficulty]);
				// sped up step by step, then no key across: it keeps its speed until it bounces
				const int32 Key = Way > 0 ? UGH_LOGIC_KEY_RIGHT : UGH_LOGIC_KEY_LEFT;
				ugh_logic_copter_danger Last = Game.Danger();
				int32 Loudness = 0;
				bool bBounced = false;
				for (int32 Step = 0; Step < MostSteps && !Game.bCrashed && !bBounced; ++Step)
				{
					Game.Keep(Way > 0 ? RightY : LeftY);
					Game.Hold(Key, FMath::Abs(Game.Danger().speed_x) < Steps * SteerStep);
					Last = Game.Danger();
					Loudness = UghWarning::Loudness(Last);
					Game.Next();
					bBounced = Game.Danger().speed_x * Way < 0 || (Last.speed_x != 0 && Game.Danger().speed_x == 0);
				}
				TestEqual(Case + TEXT(": the speed it hits with"), Last.speed_x, Way * Steps * SteerStep);
				TestTrue(Case + TEXT(": rock ahead"), Last.rock_x != 0);
				TestTrue(Case + TEXT(": it hit the rock"), Game.bCrashed || bBounced);
				TestEqual(Case + TEXT(": warned exactly when it crashes"), Loudness > 0, Game.bCrashed);
			}
		}

		// up at full speed into the top of the screen: it stops there, no crash, no warning
		{
			const FString Case = FString::Printf(TEXT("difficulty %d, up"), Difficulty);
			FGame Game;
			if (TestTrue(Case + TEXT(": the play begins"), Game.Start(Difficulty)) &&
				TestTrue(Case + TEXT(": at its height"), Game.Settle(LeftY)))
			{
				int32 Fastest = 0;
				bool bWarned = false;
				for (int32 Step = 0; Step < 400 && !Game.bCrashed; ++Step)
				{
					Game.Hold(UGH_LOGIC_KEY_UP, true);
					Game.Next();
					Fastest = FMath::Max(Fastest, Game.Danger().impact_y);
					bWarned |= UghWarning::Loudness(Game.Danger()) > 0;
				}
				TestTrue(Case + TEXT(": fast enough to crash into rock"), Fastest >= Limits[Difficulty]);
				TestFalse(Case + TEXT(": no warning"), bWarned);
				TestFalse(Case + TEXT(": no crash"), Game.bCrashed);
			}
		}

		// diving onto the ground: a crash, warned before it, louder the faster
		{
			const FString Case = FString::Printf(TEXT("difficulty %d, diving"), Difficulty);
			FGame Game;
			if (TestTrue(Case + TEXT(": the play begins"), Game.Start(Difficulty)) &&
				TestTrue(Case + TEXT(": at its height"), Game.Settle(60)))
			{
				Game.Hold(UGH_LOGIC_KEY_UP, false);
				int32 Loudness = 0, Loudest = 0;
				for (int32 Step = 0; Step < 400 && !Game.bCrashed; ++Step)
				{
					Game.Hold(UGH_LOGIC_KEY_DOWN, true);
					const int32 Now = UghWarning::Loudness(Game.Danger());
					TestTrue(Case + TEXT(": louder the faster"), Now >= Loudness);
					Loudness = Now;
					Loudest = FMath::Max(Loudest, Now);
					Game.Next();
				}
				TestTrue(Case + TEXT(": a crash"), Game.bCrashed);
				TestTrue(Case + TEXT(": warned before it"), Loudness > 0);
				AddInfo(FString::Printf(TEXT("%s: the loudest %d"), *Case, Loudest));
			}
		}
	}

	// on hard into the open sea from high (FUghDunkPilot): no crash, no warning however fast
	{
		FGame Game;
		if (TestTrue(TEXT("dunk: the play begins"), Game.Start(2)))
		{
			FUghDunkPilot Pilot;
			int32 Fastest = 0;
			bool bWarned = false;
			for (int32 Step = 0; Step < MostSteps && !Game.bCrashed; ++Step)
			{
				const FUghPilotKeys Keys = Pilot.Fly(Game.Logic, Game.Previous, Game.Current);
				Game.Hold(UGH_LOGIC_KEY_UP, Keys.bUp);
				Game.Hold(UGH_LOGIC_KEY_LEFT, Keys.bLeft);
				Game.Hold(UGH_LOGIC_KEY_RIGHT, Keys.bRight);
				Game.Next();
				const ugh_logic_copter_danger Danger = Game.Danger();
				if (Pilot.HasDropped() && Danger.speed_y > 0)
				{
					Fastest = FMath::Max(Fastest, Danger.impact_y);
					bWarned |= UghWarning::Loudness(Danger) > 0;
				}
				if (Pilot.HasDropped() && Danger.speed_y < 0)
				{
					break;   // under water, floating up again
				}
			}
			TestTrue(TEXT("dunk: fast enough to crash into rock"), Fastest >= Limits[2]);
			TestFalse(TEXT("dunk: no warning"), bWarned);
			TestFalse(TEXT("dunk: no crash"), Game.bCrashed);
		}
	}

	// the loudness by the impact: none below the limit or without rock ahead, then a third of the way to the top each
	ugh_logic_copter_danger Danger{};
	Danger.crash_limit = 2300;
	Danger.rock_x = 1;
	const int32 Third = (UGH_LOGIC_COPTER_TOP_SPEED - 2300) / 3;
	const TPair<int32, int32> Loud[] = { { 2298, 0 }, { 2300, 1 }, { 2300 + Third - 1, 1 }, { 2300 + Third + 1, 2 },
		{ 2300 + 2 * Third + 1, 3 }, { UGH_LOGIC_COPTER_TOP_SPEED, 3 } };
	for (const TPair<int32, int32>& Each : Loud)
	{
		Danger.impact_x = Each.Key;
		TestEqual(FString::Printf(TEXT("loudness at %d"), Each.Key), UghWarning::Loudness(Danger), Each.Value);
	}
	Danger.rock_x = 0;
	TestEqual(TEXT("no rock ahead: none"), UghWarning::Loudness(Danger), 0);
	TestTrue(TEXT("it blinks"), UghWarning::IsLit(1, 0) && !UghWarning::IsLit(1, 0.9 / UghWarning::BlinkRates[0]));
	return true;
}

#endif
