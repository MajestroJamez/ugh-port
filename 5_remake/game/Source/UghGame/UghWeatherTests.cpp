// The water, the rain and the moods as automation tests of the editor: the water's surface is the logic's water level
// while it rises (Ugh.Water.Level), the rain falls with the wind as the logic's raindrops do (Ugh.Rain.Wind), every
// level has its mood (Ugh.Mood), the figures stand out of it (Ugh.Figures.Look).
#if WITH_DEV_AUTOMATION_TESTS

#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/WorldInitializationValues.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghCaveman.h"
#include "UghFigureLook.h"
#include "UghMood.h"
#include "UghRain.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghStage.h"
#include "UghWater.h"

namespace
{
	constexpr double FrameSeconds = 1.0 / 60;

	/** A world of the game for the actors under test, gone with it. */
	struct FTestWorld
	{
		UWorld* World = nullptr;

		/** (without physics: nothing collides, and the meshes built at run time keep no data for it) */
		FTestWorld()
		{
			const FWorldInitializationValues Values = FWorldInitializationValues().CreatePhysicsScene(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}

		~FTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** A new actor of the world, begun to play (a world without a game mode does not begin its actors). */
		template <typename TActor>
		TActor* Spawn()
		{
			TActor* Actor = World->SpawnActor<TActor>();
			if (Actor && !Actor->HasActorBegunPlay())
			{
				Actor->DispatchBeginPlay();
			}
			return Actor;
		}
	};

	/** The game data loaded into `Simulation`, a new game of one player from level `Level` (from 0) played. */
	bool Play(FAutomationTestBase& Test, FUghSimulation& Simulation, int32 Level)
	{
		const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
		FString Error;
		const bool bLoaded = Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error);
		return Test.TestTrue(TEXT("the game data: ") + Error, bLoaded)
			&& Test.TestTrue(TEXT("a new game"), Simulation.NewGame(FUghGameChoice{ 1, 1, Level }));
	}

	/** A frame of the game: on a caption a key goes on (as a player would), in the play the logic steps. */
	void Frame(FUghSimulation& Simulation)
	{
		if (Simulation.GetCurrent().phase == UGH_LOGIC_PHASE_CAPTION)
		{
			Simulation.MenuKey(UGH_LOGIC_MENU_OTHER);
		}
		Simulation.Advance(FrameSeconds);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghWaterLevelTest, "Ugh.Water.Level",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghWaterLevelTest::RunTest(const FString& Parameters)
{
	// level 3 of one player (level_id 8): its water rises from the start
	constexpr int32 RisingLevel = 2, RisingId = 8;
	constexpr double PlaySeconds = 20, MinRise = 15;
	FUghSimulation Simulation;
	if (!Play(*this, Simulation, RisingLevel))
	{
		return false;
	}
	FTestWorld Test;
	AUghWater* Water = Test.Spawn<AUghWater>();
	if (!TestNotNull(TEXT("the water"), Water))
	{
		return false;
	}
	double First = -1, Last = -1, WorstOff = 0;
	int32 Frames = 0;
	for (double Time = 0; Time < 60 && !Simulation.IsOver() && Frames < PlaySeconds / FrameSeconds; Time += FrameSeconds)
	{
		Frame(Simulation);
		const ugh_logic_view& Previous = Simulation.GetPrevious();
		const ugh_logic_view& Current = Simulation.GetCurrent();
		if (Current.phase != UGH_LOGIC_PHASE_PLAY || Previous.phase != UGH_LOGIC_PHASE_PLAY)
		{
			continue;
		}
		TestEqual(TEXT("the rising level"), Current.level_id, RisingId);
		// the water shown between the two steps, as the game mode shows it
		const double Surface = UghWater::Surface(Previous, Current, Simulation.Alpha());
		Water->Show(Surface, {});
		const double Logic = FMath::Lerp(double(Previous.water_level), double(Current.water_level), Simulation.Alpha()) /
			UghShapes::Subpixels;
		WorstOff = FMath::Max(WorstOff, FMath::Abs(Water->SurfaceZ() - UghShapes::ToWorld(0, Logic, 0).Z));
		First = First < 0 ? Logic : First;
		Last = Logic;
		++Frames;
	}
	TestTrue(FString::Printf(TEXT("%d frames of the play"), Frames), Frames > PlaySeconds / FrameSeconds / 2);
	TestTrue(FString::Printf(TEXT("the water rose (%.1f -> %.1f px)"), First, Last), First - Last > MinRise);
	TestTrue(FString::Printf(TEXT("the surface is the logic's water level (%.4f units off at worst)"), WorstOff),
		WorstOff < 1e-3);
	// the sea reaches beyond the screen's sides and towards the camera, its surface where the level's is
	const TOptional<FTransform> Box = UghWater::Box(Last);
	if (TestTrue(TEXT("the water is shown"), Box.IsSet()))
	{
		const FBox Bounds = FBox(FVector(-50), FVector(50)).TransformBy(*Box);
		TestTrue(TEXT("beyond the screen's sides"),
			Bounds.Min.X < -1000 && Bounds.Max.X > UghShapes::ToWorld(UghShapes::ScreenWidth, 0, 0).X + 1000);
		TestTrue(TEXT("towards the camera"), Bounds.Max.Y >= -UghWater::Front - 1);
		TestEqual(TEXT("its top at the surface"), Bounds.Max.Z, UghShapes::ToWorld(0, Last, 0).Z, 1e-3);
	}
	// the open sea while the camera flies in (FUghIntro): as far every way, its surface the same
	const TOptional<FTransform> Open = UghWater::Box(Last, true);
	if (TestTrue(TEXT("the open sea is shown"), Open.IsSet()))
	{
		const FBox Bounds = FBox(FVector(-50), FVector(50)).TransformBy(*Open);
		const double Middle = UghShapes::ToWorld(UghShapes::ScreenWidth / 2.0, 0, 0).X;
		TestTrue(TEXT("the open sea all around"), Bounds.Min.X <= Middle - UghWater::OpenSea + 1 &&
			Bounds.Max.X >= Middle + UghWater::OpenSea - 1 && Bounds.Min.Y <= -UghWater::OpenSea + 1 &&
			Bounds.Max.Y >= UghWater::OpenSea - 1);
		TestEqual(TEXT("its top at the surface"), Bounds.Max.Z, UghShapes::ToWorld(0, Last, 0).Z, 1e-3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRainWindTest, "Ugh.Rain.Wind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRainWindTest::RunTest(const FString& Parameters)
{
	// level 43 of one player (level_id 44): wind to the left; level 64 (level_id 23): wind to the right
	for (const int32 Level : { 42, 63 })
	{
		FUghSimulation Simulation;
		if (!Play(*this, Simulation, Level))
		{
			return false;
		}
		FTestWorld Test;
		AUghRain* Rain = Test.Spawn<AUghRain>();
		FVector2D Moved = FVector2D::ZeroVector;
		int32 Wind = 0, Drops = 0;
		for (int32 Step = 0; Step < 600 && !Simulation.IsOver(); ++Step)
		{
			Frame(Simulation);
			const ugh_logic_view& Previous = Simulation.GetPrevious();
			const ugh_logic_view& Current = Simulation.GetCurrent();
			if (Current.phase != UGH_LOGIC_PHASE_PLAY || Previous.phase != UGH_LOGIC_PHASE_PLAY)
			{
				continue;
			}
			if (Wind == 0)
			{
				Wind = Current.wind;
				Rain->Build(Simulation.GetLogic(), Current.level_id, Wind);
			}
			// the way each drop went in a step (not one that began again at the top)
			for (int32 I = 0; I < FMath::Min(Previous.raindrop_count, Current.raindrop_count); ++I)
			{
				const FVector2D Way(Current.raindrops[I][0] - Previous.raindrops[I][0],
					Current.raindrops[I][1] - Previous.raindrops[I][1]);
				if (Way.Y > 0 && Way.Size() < 8)
				{
					Moved += Way;
					++Drops;
				}
			}
		}
		const FVector2D Fall = UghRain::Fall(Wind);
		TestTrue(FString::Printf(TEXT("level %d is windy"), Level + 1), Wind != 0 && Drops > 1000);
		TestEqual(TEXT("the rain falls with the wind"), FMath::Sign(Fall.X), double(Wind));
		TestTrue(FString::Printf(TEXT("as the logic's raindrops (%.3f, %.3f vs %.3f, %.3f)"), Moved.GetSafeNormal().X,
			Moved.GetSafeNormal().Y, Fall.X, Fall.Y), Moved.GetSafeNormal().Equals(Fall, 0.01));
		// the streaks of the rain shown go that way on the screen
		Rain->Show(Simulation.GetCurrent(), UghShapes::ScreenHeight);
		const FVector2D Shown = Rain->StreakFall();
		TestTrue(FString::Printf(TEXT("the streaks fall so (%.3f, %.3f)"), Shown.X, Shown.Y), Shown.Equals(Fall, 0.01));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghMoodTest, "Ugh.Mood",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghMoodTest::RunTest(const FString& Parameters)
{
	TSet<FUghMood::EKind> Calm;
	for (int32 Level = 0; Level < 81; ++Level)
	{
		const FUghMood& Mood = UghMood::Of(Level, 0);
		Calm.Add(Mood.Kind);
		TestTrue(TEXT("the same level, the same mood"), &UghMood::Of(Level, 0) == &Mood);
		TestTrue(TEXT("a windy level is a storm"), UghMood::Of(Level, Level % 2 ? 1 : -1).Kind == FUghMood::EKind::Storm);
		TestTrue(FString::Printf(TEXT("level %d: the %s readable (exposure)"), Level + 1, Mood.Name),
			Mood.Exposure > -3 && Mood.Exposure < 3 && Mood.SunLux > 0);
		TestTrue(FString::Printf(TEXT("level %d: the figures stand out of the %s (their light, their halo)"), Level + 1,
			Mood.Name), Mood.FigureFill > 0.5f && Mood.Halo > 0 && Mood.Halo < 1);
	}
	for (const FUghMood::EKind Kind : { FUghMood::EKind::Day, FUghMood::EKind::Evening, FUghMood::EKind::Dusk,
		FUghMood::EKind::Night })
	{
		TestTrue(FString::Printf(TEXT("a calm level of mood %d"), int32(Kind)), Calm.Contains(Kind));
	}
	const FUghMood& Storm = UghMood::Of(0, 1);
	TestTrue(TEXT("the figures stand out of a storm"), Storm.FigureFill > 0.5f && Storm.Halo > 0 && Storm.Halo < 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFigureLookTest, "Ugh.Figures.Look",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The figures stand out (UghFigureLook): a person the game makes is lit by the figures' lights too and drawn into the
 * custom depth for its halo (in a copter without it); the stage's lights besides the sun light the figures alone,
 * without shadows.
 */
bool FUghFigureLookTest::RunTest(const FString& Parameters)
{
	FTestWorld Test;
	auto Marked = [this](USceneComponent* Root, bool bHalo, const TCHAR* What)
	{
		TArray<USceneComponent*> Parts;
		Root->GetChildrenComponents(true, Parts);
		int32 Primitives = 0;
		for (USceneComponent* Part : Parts)
		{
			if (const UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Part))
			{
				++Primitives;
				TestTrue(FString::Printf(TEXT("%s: %s in the scene's and the figures' light"), What, *Part->GetName()),
					Primitive->LightingChannels.bChannel0 && Primitive->LightingChannels.bChannel1);
				TestEqual(FString::Printf(TEXT("%s: %s in the custom depth"), What, *Part->GetName()),
					Primitive->bRenderCustomDepth, bHalo);
			}
		}
		TestTrue(FString::Printf(TEXT("%s has parts"), What), Primitives > 0);
	};
	FUghCaveman Caveman;
	if (Caveman.Load())
	{
		AActor* Owner = Test.Spawn<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Owner);
		Owner->SetRootComponent(Root);
		Root->RegisterComponent();
		USceneComponent* Person = Caveman.Add(Owner, 1);
		Marked(Person, true, TEXT("a passenger"));
		UghFigureLook::Mark(Person, false);
		Marked(Person, false, TEXT("a passenger in a copter"));
	}
	else
	{
		AddInfo(TEXT("no people (metahumans.ps1, fetch-assets.ps1): not checked"));
	}
	const AUghStage* Stage = Test.Spawn<AUghStage>();
	TArray<UDirectionalLightComponent*> Lights;
	Stage->GetComponents(Lights);
	int32 Scene = 0, Figures = 0;
	for (const UDirectionalLightComponent* Light : Lights)
	{
		const bool bFigures = !Light->LightingChannels.bChannel0;
		Scene += !bFigures;
		Figures += bFigures && Light->LightingChannels.bChannel1 && !Light->CastShadows
			&& Light->IndirectLightingIntensity == 0;
	}
	TestEqual(TEXT("the sun lights the scene"), Scene, 1);
	TestEqual(TEXT("the figures' lights (a fill and a rim) light the figures alone, without shadows"), Figures, 2);
	return true;
}

#endif
