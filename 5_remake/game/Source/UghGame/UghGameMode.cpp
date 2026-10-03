#include "UghGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghBackground.h"
#include "UghFigures.h"
#include "UghHud.h"
#include "UghKeyboard.h"
#include "UghPlayerController.h"
#include "UghShapes.h"
#include "UnrealClient.h"

namespace
{
	/** The camera's horizontal field of view (degrees): narrow, so the boxes look nearly flat. */
	constexpr float FieldOfView = 30.f;
	/** Room around the screen of the original. */
	constexpr double ScreenMargin = 1.08;

	/** -UghShot: when it skips the caption, pedals, takes the picture and quits (seconds of the fully shown play). */
	constexpr double ShotCaptionKeyEvery = 0.5;
	constexpr double ShotLiftFrom = 0, ShotLiftTo = 0.8, ShotAt = 1.6, ShotQuitAt = 2.2;
	constexpr double ShotTimeLimit = 120;

	FString AssetsDir()
	{
		FString Dir;
		if (FParse::Value(FCommandLine::Get(), TEXT("-UghAssets="), Dir))
		{
			return Dir;
		}
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));   // 5_remake/game
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

	const TCHAR* CommandLine = FCommandLine::Get();
	if (FParse::Value(CommandLine, TEXT("-UghShot="), ShotPath) || FParse::Param(CommandLine, TEXT("UghShot")))
	{
		ShotPath = FPaths::ConvertRelativePathToFull(
			ShotPath.IsEmpty() ? FPaths::ProjectSavedDir() / TEXT("Shots/level1.png") : ShotPath);
	}

	const FString Assets = AssetsDir();
	if (!SpriteSizes.Load(Assets / TEXT("sprites.json"), Problem) ||
		!Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Problem))
	{
		UE_LOG(LogTemp, Error, TEXT("UGH no game: %s"), *Problem);
		if (!ShotPath.IsEmpty())
		{
			Quit();   // no screenshot: shot.ps1 reports it
		}
		return;
	}
	Simulation.NewGame();
}

/** The light (a sun from the front above, the sky), the camera, the background and the figures. */
void AUghGameMode::BuildStage()
{
	UWorld* World = GetWorld();
	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-35, -100, 0));
	Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	Sun->GetLightComponent()->SetIntensity(5.f);
	// no shadows: on the stretched boxes they fall as long streaks; the grey boxes are shaded by their faces only
	Sun->GetLightComponent()->SetCastShadows(false);
	Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetAtmosphereSunLight(true);

	AActor* Atmosphere = World->SpawnActor<AActor>();
	USkyAtmosphereComponent* Sky = NewObject<USkyAtmosphereComponent>(Atmosphere);
	Sky->RegisterComponent();

	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>();
	SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
	SkyLight->GetLightComponent()->bRealTimeCapture = true;
	SkyLight->GetLightComponent()->SetIntensity(1.f);
	SkyLight->GetLightComponent()->RecaptureSky();

	Background = World->SpawnActor<AUghBackground>();
	Figures = World->SpawnActor<AUghFigures>();

	Camera = World->SpawnActor<ACameraActor>(FVector::ZeroVector, FRotator(0, -90, 0));   // looking along -Y
	Camera->GetCameraComponent()->SetFieldOfView(FieldOfView);
	Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
	if (APlayerController* Controller = World->GetFirstPlayerController())
	{
		Controller->SetViewTarget(Camera);
	}
	FitCamera();
}

void AUghGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Simulation.Advance(DeltaSeconds);
	ShowFrame();
	if (!ShotPath.IsEmpty())
	{
		TickShot(DeltaSeconds);
	}
}

void AUghGameMode::ShowFrame()
{
	const ugh_logic_view& Previous = Simulation.GetPrevious();
	const ugh_logic_view& Current = Simulation.GetCurrent();
	if (Current.level_id != BackgroundLevel)
	{
		Background->Build(Simulation.GetLogic());
		BackgroundLevel = Current.level_id;
	}
	const double Water = FMath::Lerp(double(Previous.water_level), double(Current.water_level), Simulation.Alpha());
	Background->SetWater(Water / UghShapes::Subpixels);
	Figures->Show(Previous, Current, Simulation.Alpha(), SpriteSizes);

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
	FitCamera();
}

/** The whole screen of the original in view, whatever the window's aspect. */
void AUghGameMode::FitCamera()
{
	FVector2D Viewport(16, 9);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(Viewport);
	}
	const double Aspect = Viewport.Y > 0 ? Viewport.X / Viewport.Y : 16.0 / 9.0;
	const double HalfTan = FMath::Tan(FMath::DegreesToRadians(FieldOfView / 2));
	const double Width = UghShapes::ScreenWidth * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Height = UghShapes::ScreenHeight * UghShapes::UnitsPerPixel * ScreenMargin;
	const double Distance = FMath::Max(Width / 2 / HalfTan, Height / 2 * Aspect / HalfTan);
	Camera->SetActorLocation(UghShapes::ToWorld(UghShapes::ScreenWidth / 2.0, UghShapes::ScreenHeight / 2.0, -Distance));
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

/**
 * -UghShot: skips the caption, pedals up as soon as the level is shown (the copter starts in the air and falls), saves
 * the screenshot and quits; gives up after ShotTimeLimit.
 */
void AUghGameMode::TickShot(float DeltaSeconds)
{
	const ugh_logic_view& View = Simulation.GetCurrent();
	ShotTotalTime += DeltaSeconds;
	if (ShotTotalTime > ShotTimeLimit)
	{
		UE_LOG(LogTemp, Error, TEXT("UGH shot: no screenshot after %.0f s (phase %d)"), ShotTimeLimit, View.phase);
		Quit();
		return;
	}
	if (View.phase != ShotPhase)
	{
		UE_LOG(LogTemp, Display, TEXT("UGH shot: phase %d, %.0f fps"), View.phase, DeltaSeconds > 0 ? 1 / DeltaSeconds : 0.f);
		ShotPhase = View.phase;
		ShotPhaseTime = 0;
	}
	if (View.phase == UGH_LOGIC_PHASE_CAPTION)
	{
		ShotPhaseTime += DeltaSeconds;
		if (ShotPhaseTime >= ShotCaptionKeyEvery)
		{
			FUghKeyboard::Handle(Simulation, EKeys::Enter, IE_Pressed);
			FUghKeyboard::Handle(Simulation, EKeys::Enter, IE_Released);
			ShotPhaseTime = 0;
		}
		return;
	}
	if (View.phase != UGH_LOGIC_PHASE_PLAY || View.fade < UghShapes::FadeShown)
	{
		return;
	}
	const double Before = ShotPhaseTime;
	ShotPhaseTime += DeltaSeconds;
	auto Reached = [&](double Moment) { return Before <= Moment && ShotPhaseTime > Moment; };
	if (Reached(ShotLiftFrom))
	{
		FUghKeyboard::Handle(Simulation, EKeys::Up, IE_Pressed);
	}
	if (Reached(ShotLiftTo))
	{
		FUghKeyboard::Handle(Simulation, EKeys::Up, IE_Released);
	}
	if (Reached(ShotAt))
	{
		UE_LOG(LogTemp, Display, TEXT("UGH shot %s (level_id %d, copter %d,%d)"), *ShotPath, View.level_id,
			View.copters[0].x, View.copters[0].y);
		FScreenshotRequest::RequestScreenshot(ShotPath, false, false);
	}
	if (Reached(ShotQuitAt))
	{
		Quit();
	}
}

void AUghGameMode::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}
