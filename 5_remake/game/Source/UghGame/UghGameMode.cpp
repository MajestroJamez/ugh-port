#include "UghGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghAssets.h"
#include "UghBackground.h"
#include "UghCampfire.h"
#include "UghCliffDressing.h"
#include "UghCopters.h"
#include "UghDecorations.h"
#include "UghFalls.h"
#include "UghFigures.h"
#include "UghGround.h"
#include "UghHud.h"
#include "UghJson.h"
#include "UghKeyboard.h"
#include "UghMood.h"
#include "UghPlayerController.h"
#include "UghRain.h"
#include "UghRockField.h"
#include "UghRockMesh.h"
#include "UghPadSigns.h"
#include "UghScenery.h"
#include "UghSeaStack.h"
#include "UghShapes.h"
#include "UghShot.h"
#include "UghSigns.h"
#include "UghSpeaker.h"
#include "UghStage.h"
#include "UghStreams.h"
#include "UghWater.h"
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
	bIntro = FUghIntro::bFlies && !FParse::Param(FCommandLine::Get(), TEXT("UghNoIntro"));

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
	Figures->LoadBubbles(Simulation.GetLogic(), Sprites.Count());
	Speaker->GetPlayer().Load(Assets / TEXT("sound"));
	if (!bShooting)
	{
		Speaker->Start();   // the autopilot is silent
	}
	Previewed = Menu.GetChoice();
	Simulation.Preview(Previewed);
}

/**
 * The stage (light, air, camera), the level's background and the pads' boards, the water, the springs' streams, the
 * rain, the copters, the figures, the campfire, the decorations, the rock dressing, the speaker.
 */
void AUghGameMode::BuildStage()
{
	UWorld* World = GetWorld();
	Stage = World->SpawnActor<AUghStage>();
	Background = World->SpawnActor<AUghBackground>();
	Signs = World->SpawnActor<AUghSigns>();
	Water = World->SpawnActor<AUghWater>();
	Falls = World->SpawnActor<AUghFalls>();
	SeaStack = World->SpawnActor<AUghSeaStack>();
	Rain = World->SpawnActor<AUghRain>();
	Copters = World->SpawnActor<AUghCopters>();
	Figures = World->SpawnActor<AUghFigures>();
	Campfire = World->SpawnActor<AUghCampfire>();
	Scenery = World->SpawnActor<AUghScenery>();
	Dressing = World->SpawnActor<AUghCliffDressing>();
	Speaker = World->SpawnActor<AUghSpeaker>();
	if (APlayerController* Controller = World->GetFirstPlayerController())
	{
		Controller->SetViewTarget(Stage);
	}
	Stage->SetCamera(AUghStage::Fit(UghShapes::Screen(), AUghStage::ViewportAspect()));
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
		Shot.Dress(Previous, Simulation.GetLogic());
		Shot.Dress(Current, Simulation.GetLogic());
	}
	if (Current.level_id != BackgroundLevel || Current.level != MoodLevel)
	{
		BuildLevel(Current);
	}
	FlyIntro(Current, Seconds);
	const double Surface = UghWater::Surface(Previous, Current, Simulation.Alpha());
	Background->SetWater(Surface);
	SeaStack->SetWater(Surface);
	Water->Show(Surface, UghWater::Rings(Current, Sprites, Surface), Intro.IsFlying());
	Falls->SetWater(Surface);
	Water->SetFalls(Falls->Feet(Surface));
	Rain->Show(Current, Surface);
	Stage->SetWater(Surface);
	Campfire->SetWater(Surface);
	TArray<FTransform> ClayRiders;
	Copters->Show(Previous, Current, Simulation.Alpha(), Seconds, ClayRiders);
	Figures->Show(Previous, Current, Simulation.Alpha(), Seconds, Sprites, FigureActions, ClayRiders);

	// the fade of the play; black around it (the HUD writes the captions) but after a level's flight; dimmed behind the
	// menu
	double Shown = Current.phase == UGH_LOGIC_PHASE_PLAY
		? FMath::Clamp(double(Current.fade) / UghShapes::FadeShown, 0.0, 1.0) : 0.0;
	if (bIntroScene)
	{
		Shown = FMath::Max(Shown, Intro.Shown());
	}
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
	const FUghCameraPose Game = AUghStage::Fit(CloseUp.Get(UghShapes::Screen()), AUghStage::ViewportAspect());
	Stage->SetCamera(Intro.IsFlying() ? Intro.Pose(Game, UghShapes::ToWorld(0, Surface, 0).Z) : Game);
}

void AUghGameMode::FlyIntro(const ugh_logic_view& View, double Seconds)
{
	const bool bPlay = View.phase == UGH_LOGIC_PHASE_PLAY;
	if (bPlay && View.fade >= UghShapes::FadeShown)
	{
		bIntroScene = false;   // the play's own fades again
	}
	if (bIntro && !bInMenu && View.phase == UGH_LOGIC_PHASE_CAPTION && View.level != IntroLevel)
	{
		IntroLevel = View.level;   // not again after a crash
		Intro.Start();
		bIntroScene = true;
	}
	else if (Intro.IsFlying())
	{
		if (bPlay)
		{
			Intro.Hurry();   // never into the play
		}
		Intro.Advance(Seconds);
	}
	SeaStack->Show(Intro.IsFlying());
}

/**
 * The diorama of the level the view shows: the rock coloured by its drawing, the pads' boards, its springs' streams, the
 * campfires, the decorations, the scanned rock dressing the cliff; the light and the air of its mood (UghMood), its rain.
 */
void AUghGameMode::BuildLevel(const ugh_logic_view& View)
{
	BackgroundLevel = View.level_id;
	MoodLevel = View.level;
	const ugh_logic* Logic = Simulation.GetLogic();
	const TArray<FColor> Art = LevelArt.Draw(View.level_id, Sprites);
	FUghRockField Field;
	Field.Build(Logic, Art, LevelArt.Doors(View.level_id));
	const TArray<FUghPadSign> PadSigns = View.level_id < 0 ? TArray<FUghPadSign>()
		: UghPadSigns::Plan(Logic, FUghGround(Logic, Field), LevelArt.Signs(View.level_id));
	const int32 WaterRow = View.water_level / UghShapes::Subpixels;
	const TArray<FUghStream> Streams =
		View.level_id < 0 ? TArray<FUghStream>() : UghStreams::Plan(Logic, Field, WaterRow, PadSigns);
	Field.CarveChannels(Streams);
	FUghRockMesh Rock;
	Rock.Build(Field);
	Background->Build(Rock, Art);
	Signs->Show(Background->ShowsArt() ? TArray<FUghPadSign>() : PadSigns, Sprites);   // else the drawing shows them
	Falls->Show(Streams);
	const double Started = FPlatformTime::Seconds();
	const TArray<FUghDecoration> Decorations = View.level_id < 0 ? TArray<FUghDecoration>()
		: UghDecorations::Plan(Logic, Field, View.level_id, WaterRow, PadSigns, Streams);
	UE_LOG(LogTemp, Display, TEXT("UGH decorations: %d in %.0f ms (%s), streams %d"), Decorations.Num(),
		(FPlatformTime::Seconds() - Started) * 1000, *UghDecorations::Summary(Decorations), Streams.Num());
	Campfire->Place(Decorations, View.wind);
	Scenery->Show(Decorations);
	const TArray<FUghRockPiece> Pieces = View.level_id < 0 ? TArray<FUghRockPiece>()
		: UghRockDressing::Plan(Logic, Field, View.level_id, Decorations, Streams);
	const int32 Roots =
		Pieces.FilterByPredicate([](const FUghRockPiece& Piece) { return Piece.Kind == FUghRockPiece::EKind::Root; }).Num();
	UE_LOG(LogTemp, Display, TEXT("UGH rock dressing: %d cliffs, %d roots"), Pieces.Num() - Roots, Roots);
	Dressing->Show(Pieces);
	const FUghMood& Mood = UghMood::Of(View.level, View.wind);
	UE_LOG(LogTemp, Display, TEXT("UGH mood: %s"), Mood.Name);
	Stage->SetMood(Mood, View.wind);
	Water->SetWeather(View.wind, Stage->SunDirection(), Mood.Caustics);
	Water->SetSky(UghAssets::Texture(Mood.Sky), Mood.SkySeen);
	Rain->Build(View.level_id < 0 ? nullptr : Logic, View.level_id, View.wind);
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
	if (Event == IE_Pressed)
	{
		Intro.Hurry();   // (the key goes to the logic all the same)
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
			IntroLevel = -1;
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
	Intro.Stop();
	bIntroScene = false;
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
