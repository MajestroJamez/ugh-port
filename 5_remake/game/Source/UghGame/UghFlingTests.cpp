// The passengers knocked off their pads, flung into the sea, as an automation test of the editor (Ugh.Fling).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghBetween.h"
#include "UghEffectPlayer.h"
#include "UghFling.h"
#include "UghKnockPilot.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghWater.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	/** The logic plays this long at most (steps): level 1 until the copter knocked a passenger off and it swims. */
	constexpr int32 MostSteps = 6000;
	/** Frames drawn between two steps. */
	constexpr int32 FramesAStep = 2;
	/** Near enough (units, pixels). */
	constexpr double Near = 1;

	/** A frame of the flung passenger: the logic's top (pixels), how it is seen, whether it splashed in it. */
	struct FSeenFrame
	{
		double Top = 0, Surface = 0;
		int32 X = 0;   // the logic's, 1/32 px
		TOptional<FUghFlight> Flight;
		TArray<FUghFlingSplash> Splashes;
	};

	const ugh_logic_entity* PassengerIn(const ugh_logic_view& View, int32 Index)
	{
		for (int32 I = 0; I < View.entity_count; ++I)
		{
			const ugh_logic_entity& Entity = View.entities[I];
			if (Entity.kind == UGH_LOGIC_ENTITY_PASSENGER && Entity.index == Index && Entity.sprite >= 0)
			{
				return &Entity;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFlingTest, "Ugh.Fling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The logic of level 1 (its random seed 0) with a copter flown into a passenger on its pad (FUghKnockPilot): the
 * passenger knocked off falls straight down into the sea in the logic; seen, it is flung towards the camera - from
 * where it stood, never behind the slab, further the longer it falls, a hump up at first -, splashes once where its
 * feet reach the water (not where the event of the knock is), at its place across, and is back where the logic has it
 * by the time it swims there: the fling is over, without a jump. A passenger the water rose to is not flung.
 */
bool FUghFlingTest::RunTest(const FString& Parameters)
{
	FUghSprites Sprites;
	FString Error;
	char Problem[256] = {};
	ugh_logic* Logic = Sprites.Load(Assets(), Error)
		? ugh_logic_create(TCHAR_TO_UTF8(*(Assets() / TEXT("logic/ugh-data.ugd"))), Problem, sizeof Problem) : nullptr;
	if (!TestNotNull(FString::Printf(TEXT("the game data: %s%hs"), *Error, Problem), Logic))
	{
		return false;
	}
	ON_SCOPE_EXIT { ugh_logic_destroy(Logic); };
	ugh_logic_settings Settings;
	ugh_logic_default_settings(&Settings);
	ugh_logic_new_game(Logic, &Settings);
	FUghFlings Flings;
	Flings.Load(Logic, &Sprites);
	FUghEffectPlayer Effects;
	Effects.Load(Logic, &Sprites, nullptr, &Flings);

	ugh_logic_view Previous{}, Current{};
	TArray<ugh_logic_event> Events;
	bool Held[3] = { false, false, false };   // up, left, right
	int32 Knocked = INDEX_NONE, EventSplashes = 0, AfterEnd = -1;
	double StoodTop = 0;
	TArray<FSeenFrame> Frames;
	for (int32 Step = 0; Step < MostSteps && AfterEnd < 30; ++Step)
	{
		Previous = Current;
		ugh_logic_step(Logic);
		Events.Reset();
		ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
		{
			static_cast<TArray<ugh_logic_event>*>(Context)->Add(*Event);
		}, &Events);
		ugh_logic_get_view(Logic, &Current);
		if (Current.phase == UGH_LOGIC_PHASE_CAPTION && Step % 20 == 0)
		{
			ugh_logic_menu_key(Logic, UGH_LOGIC_MENU_OTHER);
		}
		for (const ugh_logic_event& Event : Events)
		{
			if (Event.kind == UGH_LOGIC_EVENT_PASSENGER_IN_WATER && Knocked == INDEX_NONE)
			{
				Knocked = Event.entity;
				if (const ugh_logic_entity* Stood = PassengerIn(Previous, Knocked))
				{
					StoodTop = UghBetween::Pixels(Stood->x, Stood->y).Y;
				}
			}
			Flings.OnEvent(Event, Current);
			Effects.OnEvent(Event, Current);
		}
		Flings.OnView(Previous, Current);
		Effects.OnView(Previous, Current);
		for (const FUghEffectOrder& Order : Effects.TakeOrders())
		{
			EventSplashes += Order.Burst == EUghBurst::Splash ? 1 : 0;
		}
		if (Current.phase == UGH_LOGIC_PHASE_PLAY && Previous.phase == UGH_LOGIC_PHASE_PLAY)
		{
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
		}
		if (Knocked == INDEX_NONE)
		{
			continue;
		}
		for (int32 Frame = 1; Frame <= FramesAStep; ++Frame)
		{
			const double Alpha = double(Frame) / FramesAStep;
			Flings.Show(Previous, Current, Alpha, 1 / (FramesAStep * FUghSimulation::TickRate));
			const ugh_logic_entity* Now = PassengerIn(Current, Knocked);
			const ugh_logic_entity* Before = PassengerIn(Previous, Knocked);
			if (!Now)
			{
				continue;
			}
			FSeenFrame Seen;
			Seen.Top = Before ? UghBetween::Position(Before->x, Before->y, Now->x, Now->y, Alpha).Y
				: UghBetween::Pixels(Now->x, Now->y).Y;
			Seen.Surface = UghWater::Surface(Previous, Current, Alpha);
			Seen.X = Now->x;
			Seen.Flight = Flings.Of(Knocked, Seen.Top, Seen.Surface);
			Seen.Splashes = Flings.TakeSplashes();
			Frames.Add(Seen);
		}
		if (!Frames.IsEmpty() && !Frames.Last().Flight)
		{
			++AfterEnd;   // a while longer: it is not flung again
		}
	}
	if (!TestTrue(TEXT("the copter knocked a passenger off its pad"), Knocked != INDEX_NONE && !Frames.IsEmpty()) ||
		!TestTrue(TEXT("it was flung"), Frames[0].Flight.IsSet()))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("it fell far (from %.1f to the water at %.1f)"), StoodTop, Frames[0].Surface),
		Frames[0].Surface - FUghFlings::EntryBelowTop - StoodTop > 50);
	TestEqual(TEXT("no splash where the event of the knock is"), EventSplashes, 0);

	// in the air: from where it stood towards the camera, never behind the slab, a hump up at first
	TestTrue(TEXT("it starts where it stood"), FMath::Abs(Frames[0].Flight->Depth) < Near &&
		FMath::Abs(Frames[0].Flight->Lift) < Near);
	int32 Splash = INDEX_NONE;
	double HighestLift = 0;
	for (int32 I = 0; I < Frames.Num() && Splash == INDEX_NONE; ++I)
	{
		const FSeenFrame& Frame = Frames[I];
		if (!Frame.Splashes.IsEmpty())
		{
			Splash = I;
			break;
		}
		if (!TestTrue(FString::Printf(TEXT("frame %d is in the air"), I), Frame.Flight && Frame.Flight->bInAir))
		{
			return false;
		}
		TestTrue(FString::Printf(TEXT("frame %d: towards the camera (%.0f)"), I, Frame.Flight->Depth),
			Frame.Flight->Depth <= 0 && (I == 0 || Frame.Flight->Depth <= Frames[I - 1].Flight->Depth));
		TestTrue(FString::Printf(TEXT("frame %d: not below the logic's height"), I), Frame.Flight->Lift >= 0);
		HighestLift = FMath::Max(HighestLift, Frame.Flight->Lift);
	}
	if (!TestTrue(TEXT("it splashed"), Splash != INDEX_NONE))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("thrown up a little (%.1f px)"), HighestLift), HighestLift > 2);

	// one splash where its feet reach the water, in front of the stone
	const FSeenFrame& In = Frames[Splash];
	const FUghFlingSplash& Splashed = In.Splashes[0];
	TestEqual(TEXT("one splash"), In.Splashes.Num(), 1);
	TestTrue(TEXT("its feet reached the water then"), In.Top + FUghFlings::EntryBelowTop >= In.Surface &&
		Frames[Splash - 1].Top + FUghFlings::EntryBelowTop < Frames[Splash - 1].Surface);
	TestTrue(FString::Printf(TEXT("the splash on the water (%s)"), *Splashed.Place.ToString()),
		FMath::Abs(Splashed.Place.Y - In.Surface) < Near);
	const double Across = UghBetween::Pixels(In.X, 0).X;
	TestTrue(TEXT("the splash at its place across"), Splashed.Place.X > Across && Splashed.Place.X < Across + 16);
	TestTrue(FString::Printf(TEXT("the splash in front of the stone (%.0f)"), Splashed.Depth),
		Splashed.Depth <= -FUghFlings::MinThrow && Splashed.Depth >= -FUghFlings::MaxThrow);
	TestTrue(TEXT("where it was seen falling"), FMath::Abs(Frames[Splash - 1].Flight->Depth - Splashed.Depth) <
		0.05 * FMath::Abs(Splashed.Depth));

	// under the water back to the logic's place: over when the logic has it afloat, without a jump
	int32 End = INDEX_NONE;
	for (int32 I = Splash; I < Frames.Num(); ++I)
	{
		const FSeenFrame& Frame = Frames[I];
		TestTrue(FString::Printf(TEXT("frame %d: no other splash"), I), I == Splash || Frame.Splashes.IsEmpty());
		TestEqual(FString::Printf(TEXT("frame %d: the logic's place across"), I), Frame.X, In.X);
		if (!Frame.Flight)
		{
			End = I;
			break;
		}
		TestTrue(FString::Printf(TEXT("frame %d: in the water"), I), !Frame.Flight->bInAir && Frame.Flight->Lift >= 0);
		TestTrue(FString::Printf(TEXT("frame %d: going back (%.0f)"), I, Frame.Flight->Depth),
			Frame.Flight->Depth <= 0 && (I == Splash || Frame.Flight->Depth >= Frames[I - 1].Flight->Depth));
	}
	if (!TestTrue(TEXT("the fling is over"), End != INDEX_NONE))
	{
		return false;
	}
	TestTrue(FString::Printf(TEXT("back where the logic has it before it is over (%.0f)"),
		Frames[End - 1].Flight->Depth), FMath::Abs(Frames[End - 1].Flight->Depth) < 0.1 * FMath::Abs(Splashed.Depth));
	const FSeenFrame& Afloat = Frames[End];
	TestTrue(TEXT("over when the logic has it afloat"),
		Afloat.Top + FUghFlings::EntryBelowTop > Afloat.Surface && Afloat.Top < Afloat.Surface);
	for (int32 I = End; I < Frames.Num(); ++I)
	{
		TestFalse(FString::Printf(TEXT("frame %d: not flung again"), I), Frames[I].Flight.IsSet());
	}

	// the water rose to a passenger on its pad: it only goes in
	ugh_logic_view Risen{};
	Risen.phase = UGH_LOGIC_PHASE_PLAY;
	Risen.water_level = 120 * UghShapes::Subpixels;
	Risen.entity_count = 1;
	const ugh_logic_entity* Swimmer = PassengerIn(Current, Knocked);   // (a sprite of a passenger in the water)
	Risen.entities[0] = { UGH_LOGIC_ENTITY_PASSENGER, 0, 100 * UghShapes::Subpixels, 117 * UghShapes::Subpixels,
		Swimmer ? Swimmer->sprite : 0, -1, 1, 0 };
	FUghFlings Calm;
	Calm.Load(Logic, &Sprites);
	Calm.OnEvent({ UGH_LOGIC_EVENT_PASSENGER_IN_WATER, -1, 0, 0 }, Risen);
	TestFalse(TEXT("a passenger the water rose to is not flung"), Calm.IsFlung(0));
	return true;
}

#endif
