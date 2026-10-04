#include "UghGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghBackground.h"
#include "UghCampfire.h"
#include "UghCopters.h"
#include "UghDecorations.h"
#include "UghFigures.h"
#include "UghHud.h"
#include "UghJson.h"
#include "UghKeyboard.h"
#include "UghLedges.h"
#include "UghPlayerController.h"
#include "UghRockMesh.h"
#include "UghScenery.h"
#include "UghShapes.h"
#include "UghShot.h"
#include "UghSpeaker.h"
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
	const FString LevelsPath = Assets / UghJson::LevelsFile;
	TSharedPtr<FJsonObject> Levels;
	if (!Sprites.Load(Assets, Problem) || !UghJson::ReadObject(LevelsPath, Levels, Problem) ||
		!LevelArt.Load(*Levels, LevelsPath, Problem) || !Passwords.Load(*Levels, LevelsPath, Problem) ||
		!Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Problem))
	{
		UE_LOG(LogTemp, Error, TEXT("UGH no game: %s"), *Problem);
		if (bShooting)
		{
			Quit();   // no screenshot: shot.ps1 reports it
		}
		return;
	}
	FigureActions.Load(Simulation.GetLogic(), Sprites.Count());
	Speaker->GetPlayer().Load(Assets / TEXT("sound"));
	if (!bShooting)
	{
		Speaker->Start();   // the autopilot is silent
	}
	Previewed = Menu.GetChoice();
	Simulation.Preview(Previewed);
}

/**
 * The stage (light, air, camera), the level's background, the copters, the figures, the campfire, the decorations,
 * the speaker.
 */
void AUghGameMode::BuildStage()
{
	UWorld* World = GetWorld();
	Stage = World->SpawnActor<AUghStage>();
	Background = World->SpawnActor<AUghBackground>();
	Copters = World->SpawnActor<AUghCopters>();
	Figures = World->SpawnActor<AUghFigures>();
	Campfire = World->SpawnActor<AUghCampfire>();
	Scenery = World->SpawnActor<AUghScenery>();
	Speaker = World->SpawnActor<AUghSpeaker>();
	if (APlayerController* Controller = World->GetFirstPlayerController())
	{
		Controller->SetViewTarget(Stage);
	}
	Stage->FitCamera();
}

void AUghGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInMenu)
	{
		Simulation.Advance(DeltaSeconds);
		PlaySounds();
		if (Simulation.IsOver())
		{
			OpenMenu();
		}
	}
	ShowFrame(DeltaSeconds);
	if (bShooting)
	{
		switch (Shot.Tick(*this, DeltaSeconds))
		{
		case FUghShot::EAction::TakeShot: FScreenshotRequest::RequestScreenshot(Shot.GetPath(), false, false); break;
		case FUghShot::EAction::Quit: Quit(); break;
		default: break;
		}
	}
}

void AUghGameMode::ShowFrame(double Seconds)
{
	ugh_logic_view Previous = Simulation.GetPrevious(), Current = Simulation.GetCurrent();
	if (bShooting)
	{
		Shot.Dress(Previous);
		Shot.Dress(Current);
	}
	if (Current.level_id != BackgroundLevel)
	{
		BuildLevel(Current);
	}
	const double Water = FMath::Lerp(double(Previous.water_level), double(Current.water_level), Simulation.Alpha());
	Background->SetWater(Water / UghShapes::Subpixels);
	Campfire->SetWater(Water / UghShapes::Subpixels);
	TArray<FTransform> ClayRiders;
	Copters->Show(Previous, Current, Simulation.Alpha(), Seconds, ClayRiders);
	Figures->Show(Previous, Current, Simulation.Alpha(), Seconds, Sprites, FigureActions, ClayRiders);

	// the fade of the play; black around it (the HUD writes the captions); dimmed behind the menu
	double Shown = Current.phase == UGH_LOGIC_PHASE_PLAY
		? FMath::Clamp(double(Current.fade) / UghShapes::FadeShown, 0.0, 1.0) : 0.0;
	if (bInMenu && Current.level_id >= 0)
	{
		Shown = MenuShown;
	}
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->SetManualCameraFade(1.f - float(Shown), FLinearColor::Black, false);
		}
	}
	const TOptional<FBox2D> CloseUp = bShooting ? Shot.CloseUp(Current) : TOptional<FBox2D>();
	Stage->FitCamera(CloseUp.Get(UghShapes::Screen()));
}

/** The diorama of the level the view shows: the rock coloured by its drawing, the campfire and the decorations. */
void AUghGameMode::BuildLevel(const ugh_logic_view& View)
{
	BackgroundLevel = View.level_id;
	const ugh_logic* Logic = Simulation.GetLogic();
	const TArray<FColor> Art = LevelArt.Draw(View.level_id, Sprites);
	FUghRockMesh Rock;
	Rock.Build(Logic, Art);
	Background->Build(Rock, Art, LevelArt.Signs(View.level_id), Sprites);
	const int32 WaterRow = View.water_level / UghShapes::Subpixels;
	const TOptional<FIntPoint> Hearth =
		View.level_id < 0 ? TOptional<FIntPoint>() : UghLedges::FindHearth(Logic, WaterRow);
	Campfire->Place(Hearth, View.wind);
	Scenery->Show(View.level_id < 0 ? TArray<FUghDecoration>()
		: UghDecorations::Plan(Logic, View.level_id, WaterRow, Hearth));
	Stage->SetWind(View.wind);
}

bool AUghGameMode::HandleKey(const FKey& Key, EInputEvent Event)
{
	if (HandleVolumeKey(Key, Event))
	{
		return true;
	}
	// the menu takes every key (U and G are in passwords)
	if (bInMenu && Simulation.IsLoaded())
	{
		if (Event == IE_Pressed)
		{
			HandleMenuKey(Key);
		}
		return true;
	}
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
	if (Event == IE_Released && Key == StartKey)
	{
		StartKey = FKey();   // a caption would take it
		return true;
	}
	FUghKeyboard::Handle(Simulation, Key, Event);
	return true;
}

void AUghGameMode::HandleMenuKey(const FKey& Key)
{
	switch (Menu.HandleKey(Key))
	{
	case FUghMenu::EAction::Play:
		if (Simulation.NewGame(Menu.GetChoice()))
		{
			Speaker->GetPlayer().OnNewGame();
			bInMenu = false;
			StartKey = Key;
		}
		break;
	case FUghMenu::EAction::Quit:
		Quit();
		break;
	default:
		if (Menu.GetChoice() != Previewed)
		{
			Previewed = Menu.GetChoice();
			Simulation.Preview(Previewed);
		}
		break;
	}
}

void AUghGameMode::OpenMenu()
{
	const ugh_logic_view& View = Simulation.GetCurrent();
	LastGame = Simulation.GetResult() == UGH_LOGIC_ALL_LEVELS_DONE
		? FString::Printf(TEXT("All levels done! Score %u"), View.score)
		: FString::Printf(TEXT("Game over in level %d, score %u"), View.level + 1, View.score);
	Speaker->GetPlayer().OnGameEnd(Simulation.GetResult());
	bInMenu = true;
	Previewed = Menu.GetChoice();
	Simulation.Preview(Previewed);
}

void AUghGameMode::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

bool AUghGameMode::HandleVolumeKey(const FKey& Key, EInputEvent Event)
{
	if (Key != EKeys::PageUp && Key != EKeys::PageDown)
	{
		return false;
	}
	if (Event == IE_Pressed)
	{
		Speaker->GetPlayer().ChangeVolume(Key == EKeys::PageUp ? 1 : -1);
	}
	return true;   // nor is its release a key of the game
}

int32 AUghGameMode::GetVolumePercent() const
{
	return Speaker ? Speaker->GetPlayer().GetVolumePercent() : 0;
}

void AUghGameMode::PlaySounds()
{
	FUghSoundPlayer& Player = Speaker->GetPlayer();
	for (const ugh_logic_event& Event : Simulation.GetEvents())
	{
		Player.OnEvent(Event);
	}
	Player.OnView(Simulation.GetCurrent());
}
