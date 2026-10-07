#include "UghGameMode.h"

#include "Camera/PlayerCameraManager.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghAssets.h"
#include "UghBackground.h"
#include "UghCampfire.h"
#include "UghTorches.h"
#include "UghCliffDressing.h"
#include "UghCopters.h"
#include "UghDecorations.h"
#include "UghEffects.h"
#include "UghEvents.h"
#include "UghFalls.h"
#include "UghFigures.h"
#include "UghFringe.h"
#include "UghGraphics.h"
#include "UghGround.h"
#include "UghHud.h"
#include "UghJson.h"
#include "UghMenuView.h"
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
	/**
	 * A part of a frame that takes longer than this is logged ("UGH slow: <part> <ms>"): the cause of a hitch, seen in
	 * any log (levels.ps1, a player's).
	 */
	constexpr double SlowPartSeconds = 0.02;
	/** A copter's splash in the sea starts this far above the surface (pixels). */
	constexpr double FoamAbove = 0.4;
	struct FSlowPart
	{
		const TCHAR* Name;
		double Started = FPlatformTime::Seconds();
		~FSlowPart()
		{
			const double Took = FPlatformTime::Seconds() - Started;
			UE_CLOG(Took > SlowPartSeconds, LogTemp, Display, TEXT("UGH slow: %s %.0f ms"), Name, Took * 1000);
		}
	};

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
	bShooting = Shot.Configure();
	CameraLog.Configure();
	LoadProfile();
	ApplySettings();
	GAreScreenMessagesEnabled = false;   // the engine's messages: the log has them, the screen is the game's
	bIntro = FUghIntro::bFlies && !FParse::Param(FCommandLine::Get(), TEXT("UghNoIntro"));

	Assets = AssetsDir();
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
	Flings.Load(Simulation.GetLogic(), &Sprites);
	Effects->GetPlayer().Load(Simulation.GetLogic(), &Sprites, &FigureActions, &Flings);
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
 * rain, the copters, the figures, the bursts of the events, the campfires, the torches, the decorations, the rock
 * dressing, the speaker.
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
	Fringe = World->SpawnActor<AUghFringe>();
	Rain = World->SpawnActor<AUghRain>();
	Copters = World->SpawnActor<AUghCopters>();
	Figures = World->SpawnActor<AUghFigures>();
	Effects = World->SpawnActor<AUghEffects>();
	Campfire = World->SpawnActor<AUghCampfire>();
	Torches = World->SpawnActor<AUghTorches>();
	Scenery = World->SpawnActor<AUghScenery>();
	Dressing = World->SpawnActor<AUghCliffDressing>();
	Speaker = World->SpawnActor<AUghSpeaker>();
	if (APlayerController* Controller = World->GetFirstPlayerController())
	{
		Controller->SetViewTarget(Stage);
	}
	Stage->SetCamera(AUghStage::Play(AUghStage::ViewportAspect()));
}

void AUghGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bInMenu)
	{
		{
			FSlowPart Part{ TEXT("logic") };
			Simulation.Advance(DeltaSeconds);
		}
		FSlowPart Part{ TEXT("events") };
		PlayEvents();
		NoteLevel(Simulation.GetCurrent());
		if (Simulation.IsOver())
		{
			OpenMenu();
		}
	}
	{
		FSlowPart Part{ TEXT("frame") };
		ShowFrame(DeltaSeconds);
	}
	if (bShooting)
	{
		switch (Shot.Tick(*this, DeltaSeconds))
		{
		case FUghShot::EAction::TakeShot:   // with the screen of AUghHud
			FScreenshotRequest::RequestScreenshot(Shot.GetPath(), true, false);
			break;
		case FUghShot::EAction::Quit: Quit(); break;
		default: break;
		}
	}
}

void AUghGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	CameraLog.Flush();
	Super::EndPlay(Reason);
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
		FSlowPart Part{ TEXT("level built") };
		BuildLevel(Current);
		CameraLog.Note(TEXT("level built"));
	}
	FlyIntro(Current, Seconds);
	// behind the menu the camera swings around the stone over the open sea
	const bool bMenuView = bInMenu && Current.level_id >= 0;
	MenuTime = bMenuView ? MenuTime + Seconds : 0;
	// the stone the level is carved into: flown to, seen around the level in the play (its edges), behind the menu;
	// shown all along from the first level on - hiding it at the end of the flight was a hitch and the light changed
	const bool bStone = SeaStack->IsShown() || Current.level_id >= 0;
	if (CameraLog.IsOn() && SeaStack->IsShown() != bStone)
	{
		CameraLog.Note(bStone ? TEXT("stone shown") : TEXT("stone hidden"));
	}
	SeaStack->Show(bStone);
	// the passengers knocked off their pads flung into the sea: their splashes where their feet reach the water
	Flings.Show(Previous, Current, Simulation.Alpha(), Seconds);
	for (const FUghFlingSplash& Splash : Flings.TakeSplashes())
	{
		Effects->GetPlayer().Order(EUghBurst::Plunge, Splash.Place, Current, Splash.Scale, Splash.Depth);
	}
	// the copters falling into the sea: a splash where they meet it, foam where they come up again (a little above
	// the surface: the foam lying on it at its very height was hidden by the water)
	Dunks.Show(Previous, Current, Simulation.Alpha(), Seconds);
	for (const FUghDunkSplash& Splash : Dunks.TakeSplashes())
	{
		Effects->GetPlayer().Order(Splash.bSurfacing ? EUghBurst::Boil : EUghBurst::Dunk,
			Splash.Place - FVector2D(0, FoamAbove), Current, Splash.Scale);
	}
	const double Surface = UghWater::Surface(Previous, Current, Simulation.Alpha());
	Background->SetWater(Surface);
	SeaStack->SetWater(Surface);
	// the open sea all along (no switch)
	Water->Show(Surface, UghWater::Rings(Current, Sprites, Surface, &Flings, Dunks.Stirs(Surface)), true);
	Falls->SetWater(Surface);
	Water->SetFalls(Falls->Feet(Surface));
	Rain->Show(Current, Surface);
	Stage->SetWater(Surface);
	Campfire->SetWater(Surface);
	Torches->SetWater(Surface);
	TArray<FTransform> ClayRiders;
	{
		FSlowPart Part{ TEXT("copters") };
		Copters->Show(Previous, Current, Simulation.Alpha(), Seconds, ClayRiders, &Dunks);
	}
	{
		FSlowPart Part{ TEXT("figures") };
		Figures->Show(Previous, Current, Simulation.Alpha(), Seconds, Sprites, FigureActions, ClayRiders, &Flings);
	}
	if (bShooting)
	{
		HoldShotEffect(Current);
	}
	// the plants at the stone's edges bend away from the copters, a bump orders leaves
	{
		FSlowPart Part{ TEXT("fringe") };
		Fringe->Show(Previous, Current, Simulation.Alpha(), Seconds, Effects->GetPlayer());
	}
	{
		FSlowPart Part{ TEXT("effects") };
		Effects->Show(Previous, Current, Simulation.Alpha(), Seconds);
	}

	// the fade of the play; black around it (the HUD shows the captions) but after a level's flight; all of the stone
	// behind the menu
	double Shown = Current.phase == UGH_LOGIC_PHASE_PLAY
		? FMath::Clamp(double(Current.fade) / UghShapes::FadeShown, 0.0, 1.0) : 0.0;
	if (bIntroScene)
	{
		Shown = FMath::Max(Shown, Intro.Shown());
	}
	if (bMenuView)
	{
		Shown = 1;
	}
	// the stone's jungle: in the flight until it is out of its view, behind the menu; not in the play, which does not see
	// it (thousands of swaying plants) - in black at once, else a few kinds a frame
	SeaStack->ShowJungle(bMenuView || (Intro.IsFlying() && Intro.GetTime() < AUghSeaStack::JungleOutOfView), Shown <= 0);
	if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
	{
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->SetManualCameraFade(1.f - float(Shown), FLinearColor::Black, false);
		}
	}
	const TOptional<FBox2D> CloseUp = bShooting ? Shot.CloseUp(Current, ShotLook, ShotAround) : TOptional<FBox2D>();
	const FUghCameraPose Game = CloseUp ? AUghStage::Fit(*CloseUp, AUghStage::ViewportAspect())
		: AUghStage::Play(AUghStage::ViewportAspect());
	const double SeaZ = UghShapes::ToWorld(0, Surface, 0).Z;
	const FUghCameraPose Pose = Intro.IsFlying() ? Intro.Pose(Game, SeaZ)
		: bMenuView ? UghMenuView::At(Game, SeaZ, MenuTime) : Game;
	Stage->SetCamera(Pose);
	CameraLog.Record(Seconds, Pose, Current.phase, Intro);
}

void AUghGameMode::FlyIntro(const ugh_logic_view& View, double Seconds)
{
	const bool bPlay = View.phase == UGH_LOGIC_PHASE_PLAY;
	if (bPlay && View.fade >= UghShapes::FadeShown)
	{
		bIntroScene = false;   // the play's own fades again
	}
	const bool bFlies = bIntro && Profile.Settings.bIntro;
	if (bFlies && !bInMenu && View.phase == UGH_LOGIC_PHASE_CAPTION && View.level != IntroLevel)
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
}

/**
 * The diorama of the level the view shows: the rock coloured by its drawing, the pads' boards, its springs' streams, the
 * campfires and torches, the decorations, the scanned rock dressing the cliff; the light and the air of its mood
 * (UghMood), its rain.
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
	const FUghMood& Mood = UghMood::Of(View.level, View.wind);
	// what a shot wants to look at
	const FUghDecoration* Looked = Decorations.FindByPredicate([this](const FUghDecoration& Decoration)
	{
		return Shot.GetLook() == UghDecorations::Name(Decoration.Kind);
	});
	ShotLook = Looked ? FVector2D(Looked->X, Looked->Y - Looked->Height / 2) : TOptional<FVector2D>();
	ShotAround = FUghShot::LookAround;
	Campfire->Place(Decorations, View.wind, Mood.FireLight);
	Torches->Place(Decorations, View.wind, Mood.FireLight);
	Scenery->Show(Decorations);
	const TArray<FUghRockPiece> Pieces = View.level_id < 0 ? TArray<FUghRockPiece>()
		: UghRockDressing::Plan(Logic, Field, View.level_id, Decorations, Streams);
	const int32 Roots =
		Pieces.FilterByPredicate([](const FUghRockPiece& Piece) { return Piece.Kind == FUghRockPiece::EKind::Root; }).Num();
	UE_LOG(LogTemp, Display, TEXT("UGH rock dressing: %d cliffs, %d roots"), Pieces.Num() - Roots, Roots);
	Dressing->Show(Pieces);
	UE_LOG(LogTemp, Display, TEXT("UGH mood: %s"), Mood.Name);
	Stage->SetMood(Mood, View.wind);
	Water->SetWeather(View.wind, Stage->SunDirection(), Mood.Caustics);
	Water->SetSky(UghAssets::Texture(Mood.Sky), Mood.SkySeen);
	Rain->Build(View.level_id < 0 ? nullptr : Logic, View.level_id, View.wind);
	if (View.level_id >= 0 && !bInMenu)
	{
		// the people the play may show, made now in the black (a MetaHuman made in the play was a hitch)
		FSlowPart Part{ TEXT("people stocked") };
		Figures->Stock();
		Copters->Stock();
		// (a flung passenger's splash, FUghFlings; a copter falling into the sea and coming up, FUghDunks)
		Effects->Stock({ EUghBurst::Plunge, EUghBurst::Dunk, EUghBurst::Boil });
	}
}

bool AUghGameMode::HandleKey(const FKey& Key, EInputEvent Event, FInputDeviceId Device)
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
	if (HandleFrontendKey(Key, Event))
	{
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
	Controls.Handle(Simulation, Key, Event, Device);
	return true;
}

void AUghGameMode::HandleMenuKey(const FKey& Key)
{
	const FKey MenuKey = FUghControls::MenuKeyOf(Key);
	if (!MenuKey.IsValid())
	{
		return;
	}
	DisplayOptions = UghGraphics::Options();   // the window now (the settings show it)
	switch (Menu.HandleKey(MenuKey))
	{
	case FUghMenu::EAction::Play:
		if (Simulation.NewGame(Menu.GetChoice()))
		{
			Speaker->GetPlayer().OnNewGame();
			bInMenu = false;
			StartKey = Key;
			IntroLevel = -1;
			Playing = Menu.GetChoice();
			Controls.Reset();
		}
		break;
	case FUghMenu::EAction::Quit:
		Quit();
		break;
	case FUghMenu::EAction::Save:
		ApplySettings();
		SaveProfile();
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
	FUghGameEnd End{ Playing, View.level, View.score, Simulation.GetResult() == UGH_LOGIC_ALL_LEVELS_DONE };
	if (bShooting)
	{
		Shot.DressEnd(End);
	}
	Menu.ShowEnd(End);
	Speaker->GetPlayer().OnGameEnd(Simulation.GetResult());
	bInMenu = true;
	Intro.Stop();
	Effects->Clear();
	bIntroScene = false;
	Previewed = Menu.GetChoice();
	Simulation.Preview(Previewed);
}

void AUghGameMode::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void AUghGameMode::PlayEvents()
{
	UghEvents::Play(Simulation.GetEvents(), Simulation.GetPrevious(), Simulation.GetCurrent(), Speaker->GetPlayer(),
		Effects->GetPlayer(), &Flings);
}

void AUghGameMode::HoldShotEffect(const ugh_logic_view& View)
{
	const TOptional<EUghBurst> Burst = UghBursts::Find(Shot.GetEffect());
	if (!Burst || View.phase != UGH_LOGIC_PHASE_PLAY || Effects->IsHolding())
	{
		return;
	}
	const FVector2D Place = Effects->GetPlayer().ShotPlace(*Burst, View);
	const UghBursts::FBurst& Shown = UghBursts::Get(*Burst);
	Effects->Hold(*Burst, Place, View, Shot.GetEffectAge().Get(Shown.ShotAge));
	ShotLook = Place;
	ShotAround = Shown.Extent;
}
