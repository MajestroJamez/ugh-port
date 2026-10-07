// A copter falling into the sea, as an automation test of the editor (Ugh.Dunk).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghBetween.h"
#include "UghDunk.h"
#include "UghKnockPilot.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghWater.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	/** The logic plays this long at most (steps); frames drawn between two steps. */
	constexpr int32 MostSteps = 8000, FramesAStep = 2;
	/** Seconds watched after the copter came up again. */
	constexpr double Watched = 4;
	/** Near enough (pixels); the most a bob may move in a frame (pixels). */
	constexpr double Near = 0.5, MostBobStep = 0.4;

	/** A frame of copter 0: where the logic and the frame have it, how it is seen besides, what splashed in it. */
	struct FSeenFrame
	{
		double Waterline = 0, Surface = 0, Middle = 0;   // pixels
		int32 Depth = 0;   // the logic's (pixels under the surface)
		FUghCopterBob Bob;
		int32 Stirs = 0;
		TArray<FUghDunkSplash> Splashes;
	};

	ugh_logic_view CopterView(int32 Y, int32 WaterLevel)
	{
		ugh_logic_view View{};
		View.phase = UGH_LOGIC_PHASE_PLAY;
		View.copter_count = 1;
		View.copters[0].x = 100 * UghShapes::Subpixels;
		View.copters[0].y = Y;
		View.water_level = WaterLevel;
		return View;
	}

	/** Splashes of copter 0 going from `Y0` to `Y1` (1/32 px) in `Steps` steps while the water goes from `W0` to `W1`. */
	int32 SplashesOf(int32 Y0, int32 Y1, int32 W0, int32 W1, int32 Steps)
	{
		FUghDunks Dunks;
		int32 Count = 0;
		ugh_logic_view Previous = CopterView(Y0, W0);
		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			const ugh_logic_view Current =
				CopterView(Y0 + (Y1 - Y0) * Step / Steps, W0 + (W1 - W0) * Step / Steps);
			for (int32 Frame = 1; Frame <= FramesAStep; ++Frame)
			{
				Dunks.Show(Previous, Current, double(Frame) / FramesAStep, 1 / (FramesAStep * FUghSimulation::TickRate));
				Count += Dunks.TakeSplashes().FilterByPredicate([](const FUghDunkSplash& Each)
				{
					return !Each.bSurfacing;
				}).Num();
			}
			Previous = Current;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghDunkTest, "Ugh.Dunk",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The logic of level 1 (its random seed 0) with copter 0 flown over open water and let go (FUghDunkPilot): in the logic
 * it falls into the sea - no crash, no life lost -, goes under, braked, floats up and stops afloat. Seen: one splash
 * at the frame its waterline meets the surface, at its middle on the surface, the water churning there a while; it is
 * where the logic has it, bobbing only once it came up (foaming once), a little, without a jump, dying away. The water
 * rising to a copter on a pad and a copter sinking slowly into it splash not.
 */
bool FUghDunkTest::RunTest(const FString& Parameters)
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
	FUghDunks Dunks;
	FUghDunkPilot Pilot;

	ugh_logic_view Previous{}, Current{};
	bool Held[3] = { false, false, false };   // up, left, right
	bool bCrashed = false, bPlayed = false;
	int32 Surfaced = INDEX_NONE;   // the frame the logic had it afloat again
	TArray<FSeenFrame> Frames;
	for (int32 Step = 0; Step < MostSteps; ++Step)
	{
		Previous = Current;
		ugh_logic_step(Logic);
		ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
		{
			*static_cast<bool*>(Context) |= Event->kind == UGH_LOGIC_EVENT_COPTER_CRASHED;
		}, &bCrashed);
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
		const FUghPilotKeys Keys = Pilot.Fly(Logic, Previous, Current);
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
		for (int32 Frame = 1; Frame <= FramesAStep; ++Frame)
		{
			const double Alpha = double(Frame) / FramesAStep;
			Dunks.Show(Previous, Current, Alpha, 1 / (FramesAStep * FUghSimulation::TickRate));
			const ugh_logic_copter& Before = Previous.copters[0];
			const ugh_logic_copter& Now = Current.copters[0];
			const FVector2D Seen = UghBetween::Position(Before.x, Before.y, Now.x, Now.y, Alpha);
			FSeenFrame Each;
			Each.Surface = UghWater::Surface(Previous, Current, Alpha);
			Each.Waterline = Seen.Y + FUghDunks::Waterline;
			Each.Middle = Seen.X + (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
			Each.Depth = FUghDunks::DepthOf(Now.y, Current.water_level);
			Each.Bob = Dunks.Of(0);
			Each.Stirs = Dunks.Stirs(Each.Surface).Num();
			Each.Splashes = Dunks.TakeSplashes();
			if (Surfaced == INDEX_NONE && Frames.Num() > 0 && Frames.Last().Depth > 0 && Each.Depth == 0)
			{
				Surfaced = Frames.Num();
			}
			Frames.Add(Each);
		}
		if (Surfaced != INDEX_NONE && Frames.Num() - Surfaced > Watched * FUghSimulation::TickRate * FramesAStep)
		{
			break;
		}
	}
	if (!TestTrue(TEXT("open water to fall into"), Pilot.GetPlace().IsSet()) ||
		!TestTrue(TEXT("the copter let go"), Pilot.HasDropped()))
	{
		return false;
	}
	AddInfo(FString::Printf(TEXT("let go at %d, %d (its corner), the water at row %d"), Pilot.GetPlace()->X,
		Pilot.GetPlace()->Y, Current.water_level / UghShapes::Subpixels));
	TestFalse(TEXT("falling into the sea is no crash"), bCrashed);

	// one splash where its waterline meets the surface
	TArray<int32> Splashed, Foamed;
	for (int32 I = 0; I < Frames.Num(); ++I)
	{
		for (const FUghDunkSplash& Splash : Frames[I].Splashes)
		{
			(Splash.bSurfacing ? Foamed : Splashed).Add(I);
		}
	}
	if (!TestEqual(TEXT("one splash"), Splashed.Num(), 1))
	{
		return false;
	}
	const int32 In = Splashed[0];
	const FSeenFrame& At = Frames[In];
	const FUghDunkSplash& Splash = *At.Splashes.FindByPredicate([](const FUghDunkSplash& Each) { return !Each.bSurfacing; });
	TestTrue(TEXT("its waterline met the surface in that frame"), In > 0 && At.Waterline >= At.Surface &&
		Frames[In - 1].Waterline < Frames[In - 1].Surface);
	TestTrue(FString::Printf(TEXT("the splash at its middle on the surface (%s)"), *Splash.Place.ToString()),
		FMath::Abs(Splash.Place.X - At.Middle) < Near && FMath::Abs(Splash.Place.Y - At.Surface) < Near);
	TestTrue(FString::Printf(TEXT("a big one (%.2f)"), Splash.Scale), Splash.Scale >= 1);
	TestTrue(TEXT("the water churns there"), At.Stirs > 0);

	// the logic: under, braked, up again, afloat
	int32 Deepest = 0;
	for (int32 I = In; I < Frames.Num(); ++I)
	{
		Deepest = FMath::Max(Deepest, Frames[I].Depth);
	}
	TestTrue(FString::Printf(TEXT("under the water (%d px)"), Deepest), Deepest > 5);
	if (!TestTrue(TEXT("afloat again"), Surfaced != INDEX_NONE && Surfaced > In))
	{
		return false;
	}
	TestTrue(TEXT("foam once, when it came up"), Foamed.Num() <= 1 && (Foamed.IsEmpty() || Foamed[0] == Surfaced));

	// seen where the logic has it, bobbing once it came up: a little, smoothly, dying away
	double Highest = 0;
	for (int32 I = 0; I < Frames.Num(); ++I)
	{
		const FUghCopterBob& Bob = Frames[I].Bob;
		if (I < Surfaced)
		{
			TestTrue(FString::Printf(TEXT("frame %d: where the logic has it before it came up"), I),
				FMath::Abs(Bob.Lift) < 1e-6);
		}
		Highest = FMath::Max(Highest, FMath::Abs(Bob.Lift));
		TestTrue(FString::Printf(TEXT("frame %d: a little (%.2f px, %.3f)"), I, Bob.Lift, Bob.Roll),
			FMath::Abs(Bob.Lift) <= FUghDunks::MaxBob && FMath::Abs(Bob.Roll) <= FUghDunks::MaxRoll);
		TestTrue(FString::Printf(TEXT("frame %d: no jump"), I),
			I == 0 || FMath::Abs(Bob.Lift - Frames[I - 1].Bob.Lift) <= MostBobStep);
	}
	TestTrue(FString::Printf(TEXT("it bobbed (%.2f px)"), Highest), Highest > 0.3);
	const FSeenFrame& Last = Frames.Last();
	TestTrue(FString::Printf(TEXT("calm at last (%.3f px, %.4f)"), Last.Bob.Lift, Last.Bob.Roll),
		FMath::Abs(Last.Bob.Lift) < 0.05 && FMath::Abs(Last.Bob.Roll) < 0.005 && Last.Depth == 0);
	TestEqual(TEXT("the water calm again"), Last.Stirs, 0);

	// no splash: the water rising to a copter on a pad, a copter sinking slowly into it
	const int32 Pixel = UghShapes::Subpixels;
	TestEqual(TEXT("the water rising to it"), SplashesOf(100 * Pixel, 100 * Pixel, 125 * Pixel, 110 * Pixel, 200), 0);
	TestEqual(TEXT("sinking slowly"), SplashesOf(100 * Pixel, 110 * Pixel, 120 * Pixel, 120 * Pixel, 100), 0);
	TestEqual(TEXT("falling fast"), SplashesOf(80 * Pixel, 110 * Pixel, 120 * Pixel, 120 * Pixel, 15), 1);
	return true;
}

#endif
