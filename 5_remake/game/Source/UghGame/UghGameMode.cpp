#include "UghGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghBackground.h"
#include "UghCampfire.h"
#include "UghFigures.h"
#include "UghHud.h"
#include "UghKeyboard.h"
#include "UghPlayerController.h"
#include "UghRockMesh.h"
#include "UghShapes.h"
#include "UghShot.h"
#include "UghStage.h"
#include "UnrealClient.h"

namespace
{
	FString AssetsDir()
	{
		FString Dir;
		if (FParse::Value(FCommandLine::Get(), TEXT("-UghAssets="), Dir))
		{
			return Dir;
		}
		// a packaged game has its own copy (package.ps1); the project 5_remake/game reads the repository's
		const FString Packaged = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("assets"));
		return FPaths::DirectoryExists(Packaged) ? Packaged
			: FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	}
}

AUghGameMode::AUghGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AUghPlayerController::StaticClass();
	HUDClass = AUghHud::StaticClass();
}

void AUghGameMode::StartPlay()
{
	Super::StartPlay();
	BuildStage();
	Upscaler.ChooseBest();

	bShooting = Shot.Configure();

	const FString Assets = AssetsDir();
	if (!Sprites.Load(Assets, Problem) || !LevelArt.Load(Assets, Problem) ||
		!Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Problem))
	{
		UE_LOG(LogTemp, Error, TEXT("UGH no game: %s"), *Problem);
		if (bShooting)
		{
			Quit();   // no screenshot: shot.ps1 reports it
		}
		return;
	}
	Simulation.NewGame();
}

/** The stage (light, air, camera), the level's background, the figures, the campfire. */
void AUghGameMode::BuildStage()
{
	UWorld* World = GetWorld();
	Stage = World->SpawnActor<AUghStage>();
	Background = World->SpawnActor<AUghBackground>();
	Figures = World->SpawnActor<AUghFigures>();
	Campfire = World->SpawnActor<AUghCampfire>();
	if (APlayerController* Controller = World->GetFirstPlayerController())
	{
		Controller->SetViewTarget(Stage);
	}
	Stage->FitCamera();
}

void AUghGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Simulation.Advance(DeltaSeconds);
	ShowFrame();
	if (bShooting)
	{
		switch (Shot.Tick(Simulation, DeltaSeconds))
		{
		case FUghShot::EAction::TakeShot: FScreenshotRequest::RequestScreenshot(Shot.GetPath(), false, false); break;
		case FUghShot::EAction::Quit: Quit(); break;
		default: break;
		}
	}
}

void AUghGameMode::ShowFrame()
{
	const ugh_logic_view& Previous = Simulation.GetPrevious();
	const ugh_logic_view& Current = Simulation.GetCurrent();
	if (Current.level_id != BackgroundLevel)
	{
		BuildLevel(Current);
	}
	const double Water = FMath::Lerp(double(Previous.water_level), double(Current.water_level), Simulation.Alpha());
	Background->SetWater(Water / UghShapes::Subpixels);
	Campfire->SetWater(Water / UghShapes::Subpixels);
	Figures->Show(Previous, Current, Simulation.Alpha(), Sprites);

	// the fade of the play; black around it (the HUD writes the captions)
	const double Shown = Current.phase == UGH_LOGIC_PHASE_PLAY
		? FMath::Clamp(double(Current.fade) / UghShapes::FadeShown, 0.0, 1.0) : 0.0;
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->SetManualCameraFade(1.f - float(Shown), FLinearColor::Black, false);
		}
	}
	Stage->FitCamera();
}

/** The diorama of the level the view shows: the rock coloured by its drawing, the campfire where it fits. */
void AUghGameMode::BuildLevel(const ugh_logic_view& View)
{
	BackgroundLevel = View.level_id;
	FUghRockMesh Rock;
	Rock.Build(Simulation.GetLogic());
	Background->Build(Rock, LevelArt.Draw(Background, View.level_id, Sprites));
	Campfire->Place(View.level_id < 0 ? TOptional<FIntPoint>()
		: Rock.FindHearth(Simulation.GetLogic(), View.water_level / UghShapes::Subpixels));
}

bool AUghGameMode::HandleKey(const FKey& Key, EInputEvent Event)
{
	// the frontend's keys: their releases are not keys of the game either (a caption would take one)
	if (Key == EKeys::U || Key == EKeys::G)
	{
		if (Event == IE_Pressed && Key == EKeys::U)
		{
			Upscaler.Next();
		}
		else if (Event == IE_Pressed)
		{
			Upscaler.NextFrameGeneration();
		}
		return true;
	}
	if (!Simulation.IsLoaded())
	{
		return false;
	}
	if (Simulation.IsOver())
	{
		if (Event == IE_Pressed && Key == EKeys::Enter)
		{
			Simulation.NewGame();
		}
		else if (Event == IE_Pressed && Key == EKeys::Escape)
		{
			Quit();
		}
		return true;
	}
	FUghKeyboard::Handle(Simulation, Key, Event);
	return true;
}

void AUghGameMode::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
