// The stone taken, carried and dropped onto an enemy, as an automation test of the editor (Ugh.Drop).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghKnockPilot.h"
#include "UghShapes.h"
#include "UghStoneDrop.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	/** The logic plays this long at most (steps); it watches this long after the stone was let go. */
	constexpr int32 MostSteps = 9000, Watched = 140;

	struct FDropSeen
	{
		bool bCrashed = false, bDropped = false, bStunned = false;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghDropTest, "Ugh.Drop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The logic of level 1 (its random seed 0) with copter 0 flown by FUghDropPilot (the autopilot of shot.ps1 -Drop): it
 * takes the stone on its sling, flies above the tree and lets it go, without a crash; the stone falls onto the tree
 * and bounces off it (the tree drops a fruit). Seen (FUghStoneDrops) it falls out of the sling straight down, never
 * up, and is nearly where the logic has it when it bounces.
 */
bool FUghDropTest::RunTest(const FString& Parameters)
{
	char Problem[256] = {};
	ugh_logic* Logic = ugh_logic_create(TCHAR_TO_UTF8(*(Assets() / TEXT("logic/ugh-data.ugd"))), Problem, sizeof Problem);
	if (!TestNotNull(FString::Printf(TEXT("the game data: %hs"), Problem), Logic))
	{
		return false;
	}
	ON_SCOPE_EXIT { ugh_logic_destroy(Logic); };
	ugh_logic_settings Settings;
	ugh_logic_default_settings(&Settings);
	ugh_logic_new_game(Logic, &Settings);
	FUghDropPilot Pilot;
	ugh_logic_view Previous{}, Current{};
	bool Held[4] = { false, false, false, false };   // up, left, right, fire
	bool bPlayed = false, bHung = false, bBounced = false, bFalling = false;
	FUghStoneDrops Drops;
	double SeenTop = 0;
	FDropSeen Seen;
	int32 DroppedAt = INDEX_NONE;
	for (int32 Step = 0; Step < MostSteps; ++Step)
	{
		Previous = Current;
		ugh_logic_step(Logic);
		ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
		{
			FDropSeen& Into = *static_cast<FDropSeen*>(Context);
			Into.bCrashed |= Event->kind == UGH_LOGIC_EVENT_COPTER_CRASHED;
			Into.bDropped |= Event->kind == UGH_LOGIC_EVENT_PASSENGER_DROPPED;
			Into.bStunned |= Event->kind == UGH_LOGIC_EVENT_ENEMY_STUNNED || Event->kind == UGH_LOGIC_EVENT_TREE_DROP;
		}, &Seen);
		ugh_logic_get_view(Logic, &Current);
		if (Current.phase == UGH_LOGIC_PHASE_CAPTION && Step % 20 == 0)
		{
			ugh_logic_menu_key(Logic, UGH_LOGIC_MENU_OTHER);
		}
		const bool bPlay = Current.phase == UGH_LOGIC_PHASE_PLAY && Previous.phase == UGH_LOGIC_PHASE_PLAY;
		if (bPlayed && !bPlay)
		{
			break;   // (the attempt ended: a crash)
		}
		bPlayed |= bPlay;
		if (!bPlay)
		{
			continue;
		}
		bHung |= Current.copters[0].destination == -1;
		const FUghDropKeys Keys = Pilot.Fly(Logic, Previous, Current);
		Drops.Begin();
		for (int32 I = 0; I < Current.entity_count; ++I)
		{
			ugh_logic_sprite Info;
			const ugh_logic_entity& Entity = Current.entities[I];
			const bool bNamed = Entity.kind == UGH_LOGIC_ENTITY_PASSENGER && Entity.sprite >= 0 &&
				ugh_logic_get_sprite(Logic, Entity.sprite, &Info);
			const bool bDrops = bNamed && FCStringAnsi::Strcmp(Info.name, "droppedPassenger") == 0;
			const bool bBounces = bNamed && FCStringAnsi::Strcmp(Info.name, "bouncedPassenger") == 0;
			if (!bDrops && !bBounces)
			{
				continue;
			}
			// seen: out of the sling, never up while it falls, nearly where the logic has it when it bounces
			const double Top = Entity.y / double(UghShapes::Subpixels);
			const double Below = Drops.Below(Entity.index, Top, true);
			if (!bFalling)
			{
				TestTrue(FString::Printf(TEXT("let go, seen in the sling (%.1f px below)"), Below),
					FMath::IsNearlyEqual(Below, FUghStoneDrops::SlingBelow(), 0.01) && Below > 10);
			}
			else if (bDrops)
			{
				TestTrue(TEXT("seen falling, never up"), Top + Below >= SeenTop - 0.01);
			}
			if (bBounces && !bBounced)
			{
				TestTrue(FString::Printf(TEXT("seen hitting it (%.1f px below)"), Below), Below < 4);
			}
			SeenTop = Top + Below;
			bFalling = true;
			bBounced |= bBounces;
		}
		Drops.End();
		const bool Wanted[4] = { Keys.bUp, Keys.bLeft, Keys.bRight, Keys.bFire };
		const int32 LogicKeys[4] = { UGH_LOGIC_KEY_UP, UGH_LOGIC_KEY_LEFT, UGH_LOGIC_KEY_RIGHT, UGH_LOGIC_KEY_FIRE };
		for (int32 Key = 0; Key < 4; ++Key)
		{
			if (Held[Key] != Wanted[Key])
			{
				ugh_logic_key(Logic, 0, LogicKeys[Key], Wanted[Key] ? 1 : 0);
				Held[Key] = Wanted[Key];
			}
		}
		if (Pilot.IsLost())
		{
			break;
		}
		if (DroppedAt == INDEX_NONE && Pilot.HasDropped())
		{
			DroppedAt = Step;
		}
		if (DroppedAt != INDEX_NONE && Step > DroppedAt + Watched)
		{
			break;
		}
	}
	TestTrue(TEXT("the level played"), bPlayed);
	TestFalse(TEXT("the pilot found the stone, the enemy and its ways"), Pilot.IsLost());
	TestTrue(TEXT("the copter took the stone on its sling"), bHung);
	TestTrue(TEXT("it let the stone go"), Seen.bDropped && Pilot.HasDropped());
	TestTrue(TEXT("the stone hit the enemy it fell onto (stunned it, shook a fruit off the tree)"), Seen.bStunned);
	TestTrue(TEXT("the stone bounced off it"), bBounced);
	TestFalse(TEXT("no crash"), Seen.bCrashed);
	return true;
}

#endif
