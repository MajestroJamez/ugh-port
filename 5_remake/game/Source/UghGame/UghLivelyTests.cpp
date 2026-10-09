// The lively passengers as automation tests of the editor (Ugh.Figures.Lively.States, .Level).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghBetween.h"
#include "UghFigureActions.h"
#include "UghFigurePlace.h"
#include "UghKnockPilot.h"
#include "UghLively.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghSprites.h"

namespace
{
	const FString LivelyAssets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	constexpr double FrameSeconds = 1.0 / 60;
	/** A passenger's sprite (pixels): its corner, its size. */
	const FVector2D SpriteAt(100, 100);
	const FIntPoint SpriteSize(10, 14);

	/** A view of the play with copter 0's corner at `Copter` (pixels) and passenger 0 at SpriteAt showing `Sprite`. */
	ugh_logic_view ViewOf(const FVector2D& Copter, int32 Sprite = 1)
	{
		ugh_logic_view View{};
		View.phase = UGH_LOGIC_PHASE_PLAY;
		View.copter_count = 1;
		View.copters[0].x = FMath::RoundToInt32(Copter.X * UghShapes::Subpixels);
		View.copters[0].y = FMath::RoundToInt32(Copter.Y * UghShapes::Subpixels);
		View.entity_count = 1;
		View.entities[0] = { UGH_LOGIC_ENTITY_PASSENGER, 0, int32(SpriteAt.X) * UghShapes::Subpixels,
			int32(SpriteAt.Y) * UghShapes::Subpixels, Sprite, -1, 1, 0 };
		return View;
	}

	FUghFigureAction Person(const TCHAR* Name, EUghFacing Facing = EUghFacing::Camera, bool bFrames = false)
	{
		FUghFigureAction Action;
		Action.Action = Name;
		Action.Facing = Facing;
		Action.bFollowsFrames = bFrames;
		Action.Frame = 2;
		Action.Frames = 4;
		return Action;
	}

	/** Shown as `Action` the figure is where it is shown as `Base` (UghFigurePlace: the logic's place). */
	bool SamePlace(const FUghFigureAction& Action, const FUghFigureAction& Base, double Phase)
	{
		return UghFigurePlace::Of(Action, SpriteAt, SpriteSize, Phase, 0).Equals(
			UghFigurePlace::Of(Base, SpriteAt, SpriteSize, Phase, 0), 1e-6);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghLivelyTest, "Ugh.Figures.Lively.States",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * A person waiting on land stands (idle: looking about) with the copters far, waves at one near, ducks under one flying
 * close and low by it - not under one standing still or landed beside it -, a duck and a wave kept a while; the logic's
 * own wave stays; delivered (shown walking after it rode hidden) it walks off cheering for a while, then walks; one
 * walking out of its door, one in the water and the stone stay as they are. Only the action's name changes: the place
 * of every one of them is the logic's (UghFigurePlace the same as without it).
 */
bool FUghLivelyTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		const TCHAR* What;
		FVector2D From, To;   // copter 0's corner in the step before and now
		EUghLively Want;
		const TCHAR* Shown;
	};
	// the passenger 100 .. 110 across, its head at 100, its feet at 114; the copter's body 5 .. 26 across, 20 high
	const FCase Cases[] = {
		{ TEXT("far"), FVector2D(250, 20), FVector2D(250, 20), EUghLively::Idle, TEXT("idle") },
		{ TEXT("near"), FVector2D(40, 60), FVector2D(41, 60), EUghLively::Wave, TEXT("wave") },
		{ TEXT("near above"), FVector2D(90, 30), FVector2D(90, 31), EUghLively::Wave, TEXT("wave") },
		{ TEXT("close and low"), FVector2D(69, 75), FVector2D(70, 75), EUghLively::Duck, TEXT("duck") },
		{ TEXT("close and low from the right"), FVector2D(121, 88), FVector2D(120, 88), EUghLively::Duck, TEXT("duck") },
		{ TEXT("close and low, still"), FVector2D(70, 75), FVector2D(70, 75), EUghLively::Wave, TEXT("wave") },
		{ TEXT("close but high"), FVector2D(70, 60), FVector2D(71, 60), EUghLively::Wave, TEXT("wave") },
		{ TEXT("landed beside"), FVector2D(70, 93), FVector2D(70, 94), EUghLively::Wave, TEXT("wave") },
	};
	for (const FCase& Case : Cases)
	{
		FUghLively Lively;
		const ugh_logic_view Before = ViewOf(Case.From), Now = ViewOf(Case.To);
		const FUghFigureAction Base = Person(TEXT("idle"));
		FUghFigureAction Action = Base;
		Lively.Begin();
		const EUghLively Shown = Lively.Apply(Before, Now, 1, FrameSeconds, Now.entities[0], &Before.entities[0],
			SpriteAt, SpriteSize, Action);
		Lively.End();
		TestEqual(FString::Printf(TEXT("a copter %s: what it shows"), Case.What), int32(Shown), int32(Case.Want));
		TestEqual(FString::Printf(TEXT("a copter %s: its action"), Case.What), FString(Action.Action),
			FString(Case.Shown));
		TestTrue(FString::Printf(TEXT("a copter %s: the logic's place"), Case.What),
			SamePlace(Action, Base, 0.3) &&
			Action.Facing == Base.Facing && Action.Door == Base.Door && Action.bInWater == Base.bInWater);
	}

	// kept a while: ducking, the copter gone at once - still ducking for DuckHold, then standing; a wave the same
	auto Run = [](FUghLively& Lively, const FVector2D& From, const FVector2D& To, const TCHAR* Name, double Seconds,
		const ugh_logic_entity* Before = nullptr, int32 Sprite = 1, EUghFacing Facing = EUghFacing::Camera)
	{
		const ugh_logic_view Previous = ViewOf(From, Sprite), Current = ViewOf(To, Sprite);
		const bool bWalk = FCString::Strcmp(Name, TEXT("walk")) == 0;
		FUghFigureAction Action = Person(Name, Facing, bWalk);
		Lively.Begin();
		Lively.Apply(Previous, Current, 1, Seconds, Current.entities[0], Before ? Before : &Previous.entities[0],
			SpriteAt, SpriteSize, Action);
		Lively.End();
		return Action;
	};
	const FVector2D Away(250, 20), Under(69, 75), Under2(70, 75), By(40, 60);
	{
		FUghLively Lively;
		Run(Lively, Under, Under2, TEXT("idle"), FrameSeconds);
		TestEqual(TEXT("the copter gone: still ducking"), FString(Run(Lively, Away, Away, TEXT("idle"), 0.3).Action),
			FString(TEXT("duck")));
		TestEqual(TEXT("then standing"), FString(Run(Lively, Away, Away, TEXT("idle"), 0.5).Action),
			FString(TEXT("idle")));
		Run(Lively, By, By, TEXT("idle"), FrameSeconds);
		TestEqual(TEXT("the copter gone: still waving"), FString(Run(Lively, Away, Away, TEXT("idle"), 0.5).Action),
			FString(TEXT("wave")));
		TestEqual(TEXT("then standing again"), FString(Run(Lively, Away, Away, TEXT("idle"), 0.6).Action),
			FString(TEXT("idle")));
		TestEqual(TEXT("the logic's wave stays"), FString(Run(Lively, Away, Away, TEXT("wave"), FrameSeconds).Action),
			FString(TEXT("wave")));
		TestEqual(TEXT("the logic's wave: a duck under a copter close and low"),
			FString(Run(Lively, Under, Under2, TEXT("wave"), FrameSeconds).Action), FString(TEXT("duck")));
	}
	// delivered: shown walking after it rode hidden - cheering for JoySeconds on the sprite's frames, then walking
	{
		FUghLively Lively;
		ugh_logic_entity Hidden = ViewOf(Away).entities[0];
		Hidden.sprite = -1;
		const FUghFigureAction Base = Person(TEXT("walk"), EUghFacing::Left, true);
		const FUghFigureAction First =
			Run(Lively, Away, Away, TEXT("walk"), FrameSeconds, &Hidden, 1, EUghFacing::Left);
		TestEqual(TEXT("delivered: cheering"), FString(First.Action), FString(TEXT("cheer")));
		TestTrue(TEXT("cheering: on the sprite's frames, the logic's place"), First.bFollowsFrames &&
			SamePlace(First, Base, 0.6));
		TestEqual(TEXT("walking on: still cheering"),
			FString(Run(Lively, Away, Away, TEXT("walk"), 1.0, nullptr, 1, EUghFacing::Left).Action),
			FString(TEXT("cheer")));
		TestEqual(TEXT("then walking"),
			FString(Run(Lively, Away, Away, TEXT("walk"), 0.7, nullptr, 1, EUghFacing::Left).Action),
			FString(TEXT("walk")));
		TestEqual(TEXT("and on"), FString(Run(Lively, Away, Away, TEXT("walk"), FrameSeconds, nullptr, 1, EUghFacing::Left)
			.Action), FString(TEXT("walk")));
		FUghLively Other;
		TestEqual(TEXT("one out of its door walks"),
			FString(Run(Other, Away, Away, TEXT("walk"), FrameSeconds, nullptr, 1, EUghFacing::Right).Action),
			FString(TEXT("walk")));
	}
	// in the water, the stone: as they are
	{
		FUghLively Lively;
		const ugh_logic_view Before = ViewOf(Under), Now = ViewOf(Under2);
		FUghFigureAction Tread = Person(TEXT("tread"));
		Tread.bInWater = true;
		Lively.Apply(Before, Now, 1, FrameSeconds, Now.entities[0], &Before.entities[0], SpriteAt, SpriteSize, Tread);
		TestEqual(TEXT("treading water"), FString(Tread.Action), FString(TEXT("tread")));
		FUghFigureAction Stone = Person(TEXT("stand"));
		Stone.Model = EUghModel::Stone;
		Lively.Apply(Before, Now, 1, FrameSeconds, Now.entities[0], &Before.entities[0], SpriteAt, SpriteSize, Stone);
		TestEqual(TEXT("the stone"), FString(Stone.Action), FString(TEXT("stand")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghLivelyLevelTest, "Ugh.Figures.Lively.Level",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The logic of level 1 (its random seed 0) with a copter flown into the first passenger waiting on land
 * (FUghKnockPilot): the passenger waits standing, waves at the copter coming near and ducks before it is knocked into
 * the water; never a duck while the copter is far. The logic is only read.
 */
bool FUghLivelyLevelTest::RunTest(const FString& Parameters)
{
	FUghSprites Sprites;
	FString Error;
	char Problem[256] = {};
	ugh_logic* Logic = Sprites.Load(LivelyAssets(), Error)
		? ugh_logic_create(TCHAR_TO_UTF8(*(LivelyAssets() / TEXT("logic/ugh-data.ugd"))), Problem, sizeof Problem) : nullptr;
	if (!TestNotNull(FString::Printf(TEXT("the game data: %s%hs"), *Error, Problem), Logic))
	{
		return false;
	}
	ON_SCOPE_EXIT { ugh_logic_destroy(Logic); };
	ugh_logic_settings Settings;
	ugh_logic_default_settings(&Settings);
	ugh_logic_new_game(Logic, &Settings);
	FUghFigureActions Actions;
	Actions.Load(Logic, Sprites.Count());
	FUghLively Lively;

	ugh_logic_view Previous{}, Current{};
	bool Held[3] = { false, false, false };   // up, left, right
	int32 Knocked = INDEX_NONE;
	TMap<int32, TArray<EUghLively>> Shown;    // by passenger, a step each until the knock
	for (int32 Step = 0; Step < 6000 && Knocked == INDEX_NONE; ++Step)
	{
		Previous = Current;
		ugh_logic_step(Logic);
		ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
		{
			int32& Into = *static_cast<int32*>(Context);
			if (Event->kind == UGH_LOGIC_EVENT_PASSENGER_IN_WATER && Into == INDEX_NONE)
			{
				Into = Event->entity;
			}
		}, &Knocked);
		ugh_logic_get_view(Logic, &Current);
		if (Current.phase == UGH_LOGIC_PHASE_CAPTION && Step % 20 == 0)
		{
			ugh_logic_menu_key(Logic, UGH_LOGIC_MENU_OTHER);
		}
		if (Current.phase != UGH_LOGIC_PHASE_PLAY || Previous.phase != UGH_LOGIC_PHASE_PLAY)
		{
			continue;
		}
		const FUghPilotKeys Keys = FUghKnockPilot::Fly(Logic, Previous, Current, Knocked != INDEX_NONE);
		const bool Wanted[3] = { Keys.bUp, Keys.bLeft, Keys.bRight };
		const int32 LogicKeys[3] = { UGH_LOGIC_KEY_UP, UGH_LOGIC_KEY_LEFT, UGH_LOGIC_KEY_RIGHT };
		for (int32 Key = 0; Key < 3; ++Key)
		{
			if (Held[Key] != Wanted[Key])
			{
				ugh_logic_key(Logic, 0, LogicKeys[Key], Wanted[Key] ? 1 : 0);
				Held[Key] = Wanted[Key];
			}
		}
		Lively.Begin();
		for (int32 I = 0; I < Current.entity_count; ++I)
		{
			const ugh_logic_entity& E = Current.entities[I];
			const ugh_logic_entity* P = nullptr;
			for (int32 J = 0; J < Previous.entity_count; ++J)
			{
				P = Previous.entities[J].kind == E.kind && Previous.entities[J].index == E.index ? &Previous.entities[J] : P;
			}
			TOptional<FUghFigureAction> Action = E.sprite >= 0 ? Actions.Of(E, P) : TOptional<FUghFigureAction>();
			if (E.kind != UGH_LOGIC_ENTITY_PASSENGER || !Action)
			{
				continue;
			}
			const FUghFigureAction Base = *Action;
			const FVector2D Place = UghBetween::Pixels(E.x, E.y);
			Shown.FindOrAdd(E.index).Add(Lively.Apply(Previous, Current, 1, 1 / FUghSimulation::TickRate, E, P, Place,
				Sprites.Size(E.sprite), *Action));
			if (!UghFigurePlace::Of(*Action, Place, Sprites.Size(E.sprite), 0.5, 0).Equals(
				UghFigurePlace::Of(Base, Place, Sprites.Size(E.sprite), 0.5, 0), 1e-6))
			{
				AddError(FString::Printf(TEXT("step %d: passenger %d is not where the logic has it"), Step, E.index));
			}
		}
		Lively.End();
	}
	if (!TestNotEqual(TEXT("the copter knocked a passenger off"), Knocked, int32(INDEX_NONE)))
	{
		return false;
	}
	const TArray<EUghLively>& Steps = Shown.FindOrAdd(Knocked);
	const int32 FirstWave = Steps.Find(EUghLively::Wave), FirstDuck = Steps.Find(EUghLively::Duck);
	AddInfo(FString::Printf(TEXT("passenger %d: %d steps seen, the first wave at %d, the first duck at %d"), Knocked,
		Steps.Num(), FirstWave, FirstDuck));
	TestTrue(TEXT("it stood waiting"), Steps.Contains(EUghLively::Idle) || Steps.Contains(EUghLively::Wave));
	TestTrue(TEXT("it waved at the copter coming"), FirstWave != INDEX_NONE);
	TestTrue(TEXT("it ducked before the knock, after waving"), FirstDuck != INDEX_NONE && FirstDuck > FirstWave);
	const int32 LastShown = Steps.FindLastByPredicate([](EUghLively State) { return State != EUghLively::None; });
	TestTrue(TEXT("ducking at the knock"), LastShown != INDEX_NONE && Steps[LastShown] == EUghLively::Duck);
	return true;
}

#endif
