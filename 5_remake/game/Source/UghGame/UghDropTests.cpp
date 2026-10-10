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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghDropGroundTest, "Ugh.Drop.Ground",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The stone in the sling never seen in the ground (UghSling): the logic of level 1 with copter 0 flown by
 * FUghDropPilot until it took the stone on its sling (its skids just above the ground), then climbing Climb pixels,
 * coming down slowly until it stands on the ground and climbing as high again. At every step the stone as
 * AUghCopters shows it (pushed aside as far as it wants) covers no solid pixel of the collision mask; it rested on the
 * ground beside the copter standing, hung freely in the air.
 */
bool FUghDropGroundTest::RunTest(const FString& Parameters)
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
	constexpr double Climb = 25, Down = 0.15;   // pixels; pixels a step coming down
	constexpr int32 Stand = 40, TakeOff = 200;   // steps standing still, climbing again
	enum class EStage : uint8 { Taking, Climbing, Landing, Leaving, Done } Stage = EStage::Taking;
	FUghDropPilot Pilot;
	ugh_logic_view Previous{}, Current{};
	bool Held[4] = { false, false, false, false };   // up, left, right, fire
	bool bPlayed = false, bRested = false, bFree = false, bLanded = false;
	double Target = 0, Still = 0, LastY = -1;
	FVector2D Push = FVector2D::ZeroVector;   // the stone pushed aside (settled at once)
	int32 InRock = 0, Hung = 0, Left = 0;
	FDropSeen Seen;
	for (int32 Step = 0; Step < MostSteps && Stage != EStage::Done; ++Step)
	{
		Previous = Current;
		ugh_logic_step(Logic);
		ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
		{
			static_cast<FDropSeen*>(Context)->bCrashed |= Event->kind == UGH_LOGIC_EVENT_COPTER_CRASHED;
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
		const ugh_logic_copter& Copter = Current.copters[0];
		const FVector2D Corner = FVector2D(Copter.x, Copter.y) / UghShapes::Subpixels;
		FUghDropKeys Keys;
		if (Copter.cargo_look != 0 && Copter.destination < 0)
		{
			// the stone as AUghCopters shows it (not swaying, pushed aside at once): its pixels' box on the screen
			++Hung;
			const double Middle = Corner.X + (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
			const double Bottom = Corner.Y + UghShapes::CopterBodyHeight;
			const double Room = UghSling::Room(Logic, Middle, Bottom);
			const FUghSlingPose Pose = UghSling::Follow(Logic, Middle, Bottom, 0, Push, 0);
			const double Across = Middle + Pose.Stone.X / UghShapes::UnitsPerPixel;
			const double StoneBottom = Bottom - Pose.Stone.Z / UghShapes::UnitsPerPixel;
			const double StoneTop = StoneBottom - UghCopterModel::StoneHeight / UghShapes::UnitsPerPixel;
			const double Half = UghCopterModel::StoneHalfWidth / UghShapes::UnitsPerPixel;
			for (int32 Column = FMath::FloorToInt32(Across - Half); Column < FMath::CeilToInt32(Across + Half); ++Column)
			{
				for (int32 Row = FMath::FloorToInt32(StoneTop); Row < FMath::CeilToInt32(StoneBottom - 0.01); ++Row)
				{
					InRock += ugh_logic_solid(Logic, Column, Row) ? 1 : 0;
				}
			}
			bRested |= Room < 0.5 && Pose.Lift > 0 && FMath::Abs(Pose.Stone.X) > UghSling::SideClear - 1;
			bFree |= Pose.Lift == 0 && Room > 12;
		}
		if (Stage == EStage::Taking)
		{
			Keys = Pilot.Fly(Logic, Previous, Current);
			if (Copter.destination < 0 && Copter.cargo_look != 0)
			{
				Stage = EStage::Climbing;
				Keys = FUghDropKeys();
				Target = Corner.Y - Climb;
			}
		}
		else if (Stage == EStage::Climbing || Stage == EStage::Landing)
		{
			if (Stage == EStage::Climbing && Corner.Y <= Target + 0.5)
			{
				Stage = EStage::Landing;
			}
			Target += Stage == EStage::Landing ? Down : 0;
			Keys.bUp = Corner.Y > Target;
			Still = Corner.Y == LastY ? Still + 1 : 0;
			if (Stage == EStage::Landing && Still >= Stand && Target > Corner.Y + 2)
			{
				bLanded = true;
				Stage = EStage::Leaving;
				Target = Corner.Y - Climb;
			}
		}
		else if (Stage == EStage::Leaving)
		{
			Keys.bUp = Corner.Y > Target;
			Stage = ++Left >= TakeOff ? EStage::Done : Stage;
		}
		LastY = Corner.Y;
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
	}
	TestTrue(TEXT("the level played"), bPlayed);
	TestTrue(FString::Printf(TEXT("the copter took the stone, landed with it and took off (%d steps)"), Hung),
		Hung > 0 && bLanded && Stage == EStage::Done);
	TestEqual(TEXT("pixels of rock the stone covered (summed over the steps)"), InRock, 0);
	TestTrue(TEXT("it rested on the ground beside the copter standing on it"), bRested);
	TestTrue(TEXT("it hung freely in the air"), bFree);
	TestFalse(TEXT("no crash"), Seen.bCrashed);
	return true;
}

#endif
