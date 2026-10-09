// The feel of an impact - the camera shaken, the pilot's gamepad rumbling - as automation tests of the editor
// (Ugh.Impact.*).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghDunk.h"
#include "UghFringe.h"
#include "UghImpacts.h"
#include "UghKnockPilot.h"
#include "UghShapes.h"
#include "UghSimulation.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	/** A frame (seconds); the logic plays at most this long (steps). */
	constexpr double Frame = 1.0 / 60;
	constexpr int32 MostSteps = 9000;

	ugh_logic_event EventOf(int32 Kind, int32 Player = -1, int32 Entity = -1)
	{
		return { Kind, Player, Entity, 0 };
	}

	/** The widest swing of the camera and the strongest rumble of each pad over `Seconds` from now. */
	struct FFelt
	{
		double Widest = 0;
		float Rumble[FUghImpacts::Pilots] = { 0, 0 };
		double FirstOffset = -1;   // the offset of the first frame
		double LastOffset = 0;     // and of the last
	};

	FFelt Feel(FUghImpacts& Impacts, double Seconds)
	{
		FFelt Felt;
		for (double Time = 0; Time < Seconds; Time += Frame)
		{
			Impacts.Advance(Frame);
			const double Offset = Impacts.Offset().Size();
			Felt.FirstOffset = Felt.FirstOffset < 0 ? Offset : Felt.FirstOffset;
			Felt.LastOffset = Offset;
			Felt.Widest = FMath::Max(Felt.Widest, Offset);
			for (int32 Pilot = 0; Pilot < FUghImpacts::Pilots; ++Pilot)
			{
				Felt.Rumble[Pilot] = FMath::Max(Felt.Rumble[Pilot], Impacts.Rumble(Pilot).Large);
			}
		}
		return Felt;
	}

	/** The logic of level 1 played step by step with pilot 1's keys, its events felt by FUghImpacts. */
	struct FGame
	{
		ugh_logic* Logic = nullptr;
		ugh_logic_view Previous{}, Current{};
		bool Held[5] = {};
		TArray<ugh_logic_event> Events;   // of the last step
		FUghImpacts Impacts;

		bool Start()
		{
			char Problem[256] = {};
			Logic = ugh_logic_create(TCHAR_TO_UTF8(*(Assets() / TEXT("logic/ugh-data.ugd"))), Problem, sizeof Problem);
			if (!Logic)
			{
				return false;
			}
			ugh_logic_settings Settings;
			ugh_logic_default_settings(&Settings);
			ugh_logic_new_game(Logic, &Settings);
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
		/** A step of the logic; its events felt; the impacts a step on. */
		void Next()
		{
			Previous = Current;
			Events.Reset();
			ugh_logic_step(Logic);
			ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
			{
				static_cast<TArray<ugh_logic_event>*>(Context)->Add(*Event);
			}, &Events);
			ugh_logic_get_view(Logic, &Current);
			for (const ugh_logic_event& Event : Events)
			{
				Impacts.OnEvent(Event);
			}
			Impacts.Advance(1 / FUghSimulation::TickRate);
		}
		bool Has(int32 Kind) const
		{
			return Events.ContainsByPredicate([Kind](const ugh_logic_event& Event) { return Event.kind == Kind; });
		}
		void Hold(int32 Key, bool bHeld)
		{
			if (Held[Key] != bHeld)
			{
				ugh_logic_key(Logic, 0, Key, bHeld ? 1 : 0);
				Held[Key] = bHeld;
			}
		}
		void Fly(const FUghPilotKeys& Keys)
		{
			Hold(UGH_LOGIC_KEY_UP, Keys.bUp);
			Hold(UGH_LOGIC_KEY_LEFT, Keys.bLeft);
			Hold(UGH_LOGIC_KEY_RIGHT, Keys.bRight);
		}
		bool IsPlaying() const { return Current.phase == UGH_LOGIC_PHASE_PLAY && Previous.phase == UGH_LOGIC_PHASE_PLAY; }
		int32 Felt() const
		{
			int32 Sum = 0;
			for (int32 Impact = 0; Impact < int32(EUghImpact::Count); ++Impact)
			{
				Sum += Impacts.Count(EUghImpact(Impact));
			}
			return Sum;
		}
	};

	/** A view of the play with copter 0 (its top left corner `At`, pixels). */
	ugh_logic_view CopterAt(const FVector2D& At)
	{
		ugh_logic_view View{};
		View.phase = UGH_LOGIC_PHASE_PLAY;
		View.level_id = 0;
		View.copter_count = 1;
		View.copters[0].x = FMath::RoundToInt32(At.X * UghShapes::Subpixels);
		View.copters[0].y = FMath::RoundToInt32(At.Y * UghShapes::Subpixels);
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghImpactFeelTest, "Ugh.Impact.Feel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The impacts felt: a crash shakes the camera and rumbles its pilot's pad (not the other's), a stone dropped onto an
 * enemy the dropper's, a fall into the sea and a bump into an edge their pilot's; the crash hardest, the bump lightest;
 * the swing starts from the rest, stays small (MostShake), dies away within half a second, the rumble too; other events
 * are not felt; with the shake off the camera stands still, the pad still rumbles.
 */
bool FUghImpactFeelTest::RunTest(const FString& Parameters)
{
	// every event of the logic but a crash and a stone on an enemy: nothing
	{
		FUghImpacts Impacts;
		for (int32 Kind = UGH_LOGIC_EVENT_LEVEL_CAPTION; Kind <= UGH_LOGIC_EVENT_BONUS_COLLECTED; ++Kind)
		{
			if (Kind != UGH_LOGIC_EVENT_COPTER_CRASHED && Kind != UGH_LOGIC_EVENT_ENEMY_STUNNED &&
				Kind != UGH_LOGIC_EVENT_TREE_DROP)
			{
				Impacts.OnEvent(EventOf(Kind, 0, 0));
			}
		}
		const FFelt Felt = Feel(Impacts, 0.5);
		TestTrue(TEXT("other events: not felt"), Felt.Widest == 0 && Felt.Rumble[0] == 0 && Felt.Rumble[1] == 0);
	}

	// each impact alone: its pilot's pad, the camera; how hard
	struct FCase
	{
		const TCHAR* Name;
		TFunction<void(FUghImpacts&)> Do;
		int32 Pilot;
		EUghImpact Impact;
	};
	const FCase Cases[] = {
		{ TEXT("a crash of pilot 2"), [](FUghImpacts& I) { I.OnEvent(EventOf(UGH_LOGIC_EVENT_COPTER_CRASHED, 1)); }, 1,
			EUghImpact::Crash },
		{ TEXT("a stone pilot 1 dropped onto an enemy"), [](FUghImpacts& I)
			{
				I.OnEvent(EventOf(UGH_LOGIC_EVENT_PASSENGER_DROPPED, 0, 3));
				I.OnEvent(EventOf(UGH_LOGIC_EVENT_ENEMY_STUNNED, -1, 1));
			}, 0, EUghImpact::StoneHit },
		{ TEXT("a stone pilot 2 dropped onto the tree"), [](FUghImpacts& I)
			{
				I.OnEvent(EventOf(UGH_LOGIC_EVENT_PASSENGER_DROPPED, 1, 3));
				I.OnEvent(EventOf(UGH_LOGIC_EVENT_TREE_DROP, -1, 0));
			}, 1, EUghImpact::StoneHit },
		{ TEXT("pilot 1 into the sea"), [](FUghImpacts& I) { I.OnDunk(0, 1); }, 0, EUghImpact::Dunk },
		{ TEXT("pilot 2 into the edge"), [](FUghImpacts& I) { I.OnBump(1, 60, false); }, 1, EUghImpact::Edge },
	};
	double LastWidest = 1e9;
	float LastRumble = 2;
	for (const FCase& Case : Cases)
	{
		FUghImpacts Impacts;
		Case.Do(Impacts);
		TestEqual(FString(Case.Name) + TEXT(": felt once"), Impacts.Count(Case.Impact), 1);
		TestEqual(FString(Case.Name) + TEXT(": the camera from the rest"), Impacts.Offset().Size(), 0.0);
		const FFelt Felt = Feel(Impacts, 1);
		TestTrue(FString::Printf(TEXT("%s: the camera shaken (%.1f units)"), Case.Name, Felt.Widest),
			Felt.Widest > 1 && Felt.Widest <= FUghImpacts::MostShake);
		TestTrue(FString(Case.Name) + TEXT(": its pilot's pad rumbles"), Felt.Rumble[Case.Pilot] > 0.1f);
		TestEqual(FString(Case.Name) + TEXT(": not the other's"), Felt.Rumble[1 - Case.Pilot], 0.f);
		TestTrue(FString(Case.Name) + TEXT(": over in a second"), Felt.LastOffset == 0 && !Impacts.IsShaking() &&
			Impacts.Rumble(Case.Pilot).IsOff());
		if (Case.Impact != EUghImpact::StoneHit || FCString::Strstr(Case.Name, TEXT("enemy")))
		{
			// the crash hardest, then the sea, the stone, the edge (the second stone as the first)
			const bool bDunkAfterStone = Case.Impact == EUghImpact::Dunk;
			TestTrue(FString(Case.Name) + TEXT(": not harder than the one before"), bDunkAfterStone ||
				(Felt.Widest <= LastWidest && Felt.Rumble[Case.Pilot] <= LastRumble));
			LastWidest = Felt.Widest;
			LastRumble = Felt.Rumble[Case.Pilot];
		}
	}

	// the swing dies away: each tenth of a second narrower than the one before
	{
		FUghImpacts Impacts;
		Impacts.OnEvent(EventOf(UGH_LOGIC_EVENT_COPTER_CRASHED, 0));
		double Before = 1e9;
		bool bDying = true;
		for (int32 Tenth = 0; Tenth < 5; ++Tenth)
		{
			const double Widest = Feel(Impacts, 0.1).Widest;
			bDying &= Widest < Before;
			Before = Widest;
		}
		TestTrue(TEXT("a crash's swing dies away"), bDying);
		const FFelt Rest = Feel(Impacts, 0.1);
		TestTrue(TEXT("still after half a second"), Rest.Widest < 0.1);
	}

	// the edge's bump along its way: sideways at the left edge, up and down at the top
	{
		FUghImpacts Side, Top;
		Side.OnBump(0, 60, false);
		Top.OnBump(0, 60, true);
		FVector2D SideWay = FVector2D::ZeroVector, TopWay = FVector2D::ZeroVector;
		for (int32 Step = 0; Step < 6; ++Step)
		{
			Side.Advance(Frame / 2);
			Top.Advance(Frame / 2);
			SideWay = FVector2D(FMath::Max(SideWay.X, FMath::Abs(Side.Offset().X)),
				FMath::Max(SideWay.Y, FMath::Abs(Side.Offset().Y)));
			TopWay = FVector2D(FMath::Max(TopWay.X, FMath::Abs(Top.Offset().X)),
				FMath::Max(TopWay.Y, FMath::Abs(Top.Offset().Y)));
		}
		TestTrue(TEXT("a bump beside: sideways"), SideWay.X > SideWay.Y);
		TestTrue(TEXT("a bump at the top: up and down"), TopWay.Y > TopWay.X);
		FUghImpacts Soft, Hard;
		Soft.OnBump(0, 15, false);
		Hard.OnBump(0, 120, false);
		TestTrue(TEXT("a faster bump harder"), Feel(Hard, 0.3).Widest > Feel(Soft, 0.3).Widest);
	}

	// many at once: never more than MostShake
	{
		FUghImpacts Impacts;
		for (int32 Each = 0; Each < 6; ++Each)
		{
			Impacts.OnEvent(EventOf(UGH_LOGIC_EVENT_COPTER_CRASHED, Each % 2));
		}
		const FFelt Felt = Feel(Impacts, 0.5);
		TestTrue(FString::Printf(TEXT("six crashes at once: at most MostShake (%.1f)"), Felt.Widest),
			Felt.Widest <= FUghImpacts::MostShake + 1e-6);
		TestTrue(TEXT("the pads at most full"), Felt.Rumble[0] <= 1 && Felt.Rumble[1] <= 1);
	}

	// the shake off: the camera still, the pad rumbles; off in the middle of a shake stops it
	{
		FUghImpacts Impacts;
		Impacts.SetShake(false);
		Impacts.OnEvent(EventOf(UGH_LOGIC_EVENT_COPTER_CRASHED, 0));
		const FFelt Felt = Feel(Impacts, 0.5);
		TestTrue(TEXT("shake off: the camera stands still"), Felt.Widest == 0 && !Impacts.IsShaking());
		TestTrue(TEXT("shake off: the pad still rumbles"), Felt.Rumble[0] > 0.1f);
		FUghImpacts Shaking;
		Shaking.OnEvent(EventOf(UGH_LOGIC_EVENT_COPTER_CRASHED, 0));
		Feel(Shaking, 0.05);
		Shaking.SetShake(false);
		TestTrue(TEXT("turned off while shaking: still at once"), Shaking.Offset().IsZero());
		Shaking.Reset();
		TestTrue(TEXT("reset: the pad still"), Shaking.Rumble(0).IsOff());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghImpactLevelTest, "Ugh.Impact.Level",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The impacts of the logic of level 1 (its random seed 0), pilot 1 flying: hovering at its start nothing is felt;
 * diving onto the ground it crashes - felt in the step of the crash, pilot 1's pad -; the stone taken and dropped onto
 * the tree (FUghDropPilot) - felt when it bounces off, no crash; flown over open water and let fall (FUghDunkPilot,
 * FUghDunks) - felt when it meets the water, once, no crash; a copter flown into the left edge and the top one
 * (FUghEdgeBumps) - a bump each time it reaches one fast, not while it stays there.
 */
bool FUghImpactLevelTest::RunTest(const FString& Parameters)
{
	// hovering, then diving onto the ground
	{
		FGame Game;
		if (TestTrue(TEXT("crash: the play begins"), Game.Start()))
		{
			for (int32 Step = 0; Step < 900; ++Step)
			{
				// hovering at 60 pixels (by where it will be 8 steps on), as Ugh.Warning dives from
				const int32 Climb = Game.Current.copters[0].y - Game.Previous.copters[0].y;
				Game.Hold(UGH_LOGIC_KEY_UP, Game.Current.copters[0].y + 8 * Climb > 60 * UghShapes::Subpixels);
				Game.Next();
			}
			TestEqual(TEXT("hovering: nothing felt"), Game.Felt(), 0);
			Game.Hold(UGH_LOGIC_KEY_UP, false);
			bool bCrashed = false;
			for (int32 Step = 0; Step < 600 && !bCrashed; ++Step)
			{
				Game.Hold(UGH_LOGIC_KEY_DOWN, true);
				Game.Next();
				bCrashed = Game.Has(UGH_LOGIC_EVENT_COPTER_CRASHED);
				TestEqual(TEXT("diving: nothing felt before the crash"), Game.Felt(), bCrashed ? 1 : 0);
			}
			TestTrue(TEXT("diving onto the ground: a crash"), bCrashed);
			TestEqual(TEXT("the crash felt"), Game.Impacts.Count(EUghImpact::Crash), 1);
			TestTrue(TEXT("pilot 1's pad rumbles at once"), Game.Impacts.Rumble(0).Large > 0.5f);
			TestTrue(TEXT("pilot 2's not"), Game.Impacts.Rumble(1).IsOff());
			TestTrue(TEXT("the camera shaken"), Game.Impacts.IsShaking());
		}
	}

	// the stone dropped onto the tree
	{
		FGame Game;
		if (TestTrue(TEXT("stone: the play begins"), Game.Start()))
		{
			FUghDropPilot Pilot;
			bool bHit = false, bCrashed = false;
			for (int32 Step = 0; Step < MostSteps && Game.IsPlaying() && !Pilot.IsLost(); ++Step)
			{
				const FUghDropKeys Keys = Pilot.Fly(Game.Logic, Game.Previous, Game.Current);
				Game.Fly(Keys);
				Game.Hold(UGH_LOGIC_KEY_FIRE, Keys.bFire);
				Game.Next();
				bCrashed |= Game.Has(UGH_LOGIC_EVENT_COPTER_CRASHED);
				if (Game.Has(UGH_LOGIC_EVENT_ENEMY_STUNNED) || Game.Has(UGH_LOGIC_EVENT_TREE_DROP))
				{
					bHit = true;
					TestEqual(TEXT("the stone on the enemy felt in its step"), Game.Impacts.Count(EUghImpact::StoneHit), 1);
					TestTrue(TEXT("by the pilot who dropped it"), Game.Impacts.Rumble(0).Large > 0.2f &&
						Game.Impacts.Rumble(1).IsOff());
					break;
				}
			}
			TestTrue(TEXT("the stone hit the enemy"), bHit);
			TestFalse(TEXT("stone: no crash"), bCrashed);
			TestEqual(TEXT("stone: nothing else felt"), Game.Felt(), 1);
		}
	}

	// into the sea
	{
		FGame Game;
		if (TestTrue(TEXT("sea: the play begins"), Game.Start()))
		{
			FUghDunkPilot Pilot;
			FUghDunks Dunks;
			bool bCrashed = false;
			int32 Splashes = 0, After = 0;
			for (int32 Step = 0; Step < MostSteps && Game.IsPlaying() && After < 280; ++Step)
			{
				Game.Fly(Pilot.Fly(Game.Logic, Game.Previous, Game.Current));
				Game.Next();
				bCrashed |= Game.Has(UGH_LOGIC_EVENT_COPTER_CRASHED);
				Dunks.Show(Game.Previous, Game.Current, 1, 1 / FUghSimulation::TickRate);
				for (const FUghDunkSplash& Splash : Dunks.TakeSplashes())
				{
					if (!Splash.bSurfacing)
					{
						Game.Impacts.OnDunk(Splash.Player, Splash.Scale);
						++Splashes;
						TestTrue(TEXT("into the sea: pilot 1's pad"), Game.Impacts.Rumble(0).Large > 0.2f);
					}
				}
				After += Splashes > 0 ? 1 : 0;   // (watched 4 s after: under, up again, afloat)
			}
			TestEqual(TEXT("into the sea: felt once"), Game.Impacts.Count(EUghImpact::Dunk), 1);
			TestFalse(TEXT("into the sea: no crash"), bCrashed);
			TestEqual(TEXT("into the sea: nothing else felt"), Game.Felt(), 1);
		}
	}

	// into the left edge fast, staying there, out and in again slowly, then up into the top
	{
		FUghEdgeBumps Edges;
		FUghImpacts Impacts;
		const double Step = 1 / FUghSimulation::TickRate;
		auto Fly = [&](const FVector2D& From, const FVector2D& To, int32 Steps)
		{
			ugh_logic_view Previous = CopterAt(From);
			for (int32 Each = 1; Each <= Steps; ++Each)
			{
				const ugh_logic_view Current = CopterAt(FMath::Lerp(From, To, double(Each) / Steps));
				Edges.See(Previous, Current, 1, Step);
				for (const FUghEdgeBumps::FBump& Bump : Edges.GetBumps())
				{
					Impacts.OnBump(Bump.Player, Bump.Speed, Bump.Edge == 2);
				}
				Impacts.Advance(Step);
				Previous = Current;
			}
		};
		const double Left = UghFringe::LeftEdge, Top = UghFringe::TopEdge;
		Fly(FVector2D(60, 80), FVector2D(Left, 80), 40);         // 70 pixels a second and more
		TestEqual(TEXT("into the left edge fast: a bump"), Impacts.Count(EUghImpact::Edge), 1);
		TestTrue(TEXT("its pilot's pad"), Impacts.Rumble(0).Large > 0);
		Fly(FVector2D(Left, 80), FVector2D(Left, 82), 60);      // staying at it
		TestEqual(TEXT("staying at it: no more"), Impacts.Count(EUghImpact::Edge), 1);
		Fly(FVector2D(Left, 82), FVector2D(Left + 3, 82), 20);
		Fly(FVector2D(Left + 3, 82), FVector2D(Left, 82), 60);  // back slowly (4 pixels a second)
		TestEqual(TEXT("back in slowly: no bump"), Impacts.Count(EUghImpact::Edge), 1);
		Fly(FVector2D(100, 30), FVector2D(100, Top), 30);       // up into the top
		TestEqual(TEXT("up into the top: a bump"), Impacts.Count(EUghImpact::Edge), 2);
	}
	return true;
}

#endif
