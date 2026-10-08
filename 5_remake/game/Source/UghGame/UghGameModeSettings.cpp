// The game's profile: the settings put into the engine, saved; the keys of the frontend.
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UghGameMode.h"
#include "UghGraphics.h"
#include "UghSpeaker.h"

void AUghGameMode::LoadProfile()
{
	DisplayOptions = UghGraphics::Options();
	ProfilePath = FUghProfile::DefaultPath();
	FString Own;
	if (bShooting && !FParse::Value(FCommandLine::Get(), TEXT("-UghProfile="), Own))
	{
		return;   // the defaults: shots look the same on any computer
	}
	FString Error;
	if (!Profile.Load(ProfilePath, Error))
	{
		UE_LOG(LogTemp, Warning, TEXT("UGH profile: %s (the defaults instead)"), *Error);
	}
	else if (!FPaths::FileExists(ProfilePath))
	{
		Profile.Settings.Quality = UghGraphics::RecommendedQuality();   // a first start: the preset for this GPU
	}
	UE_LOG(LogTemp, Display, TEXT("UGH profile %s"), *ProfilePath);
}

void AUghGameMode::ApplySettings()
{
	const FUghSettings& Settings = Profile.Settings;
	if (!Applied || Applied->Quality != Settings.Quality)
	{
		UghGraphics::ApplyQuality(Settings.Quality, GetWorld());
	}
	if (!Applied || Applied->Upscaler != Settings.Upscaler)
	{
		Upscaler.Use(Settings.Upscaler);
	}
	if (!Applied || Applied->FrameGeneration != Settings.FrameGeneration)
	{
		Upscaler.SetFrameGeneration(Settings.FrameGeneration);
	}
	const bool bDisplayChanged = !Applied || Applied->Resolution != Settings.Resolution ||
		Applied->WindowMode != Settings.WindowMode;
	if (bDisplayChanged && !bShooting)   // a shot keeps its window
	{
		UghGraphics::ApplyDisplay(Settings);
	}
	Speaker->GetPlayer().SetVolumes(Settings.Volume, Settings.Music, Settings.Effects);
	Applied = Settings;
}

void AUghGameMode::SaveProfile()
{
	FString Error;
	if (!bShooting && !Profile.Save(ProfilePath, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("UGH profile not saved: %s"), *Error);
	}
}

void AUghGameMode::NoteLevel(const ugh_logic_view& View)
{
	const bool bReached = View.level > Playing.FirstLevel && View.level < Passwords.LevelCount(Playing.Players);
	bool bChanged = false;
	if (bReached && View.level != Profile.Scores.LastLevel(Playing.Players))
	{
		Profile.Scores.SetLastLevel(Playing.Players, View.level);
		bChanged = true;
	}
	// the game went on from the level played: it is done (the level selection shows it green)
	if (View.level_id >= 0 && PlayedLevel >= 0 && View.level > PlayedLevel)
	{
		bChanged |= Profile.Scores.SetDone(Playing.Players, PlayedLevel);
		PlayedLevel = View.level;
	}
	if (bChanged)
	{
		SaveProfile();
	}
}

bool AUghGameMode::HandleVolumeKey(const FKey& Key, EInputEvent Event)
{
	if (Key != EKeys::PageUp && Key != EKeys::PageDown)
	{
		return false;
	}
	FUghSettings& Settings = Profile.Settings;
	const int32 Volume = FMath::Clamp(Settings.Volume + (Key == EKeys::PageUp ? 1 : -1) * FUghSettings::VolumeStep, 0, 100);
	if (Event == IE_Pressed && Volume != Settings.Volume)
	{
		Settings.Volume = Volume;
		ApplySettings();
		SaveProfile();
	}
	return true;   // nor is its release a key of the game
}

bool AUghGameMode::HandleFrontendKey(const FKey& Key, EInputEvent Event)
{
	// their releases are not keys of the game either (a caption would take one)
	if (Key != EKeys::U && Key != EKeys::G && Key != EKeys::F1 && Key != EKeys::Gamepad_FaceButton_Top)
	{
		return false;
	}
	if (Event != IE_Pressed)
	{
		return true;
	}
	FUghSettings& Settings = Profile.Settings;
	if (Key == EKeys::U)
	{
		Upscaler.Next();
		Settings.Upscaler = Upscaler.GetKind();
	}
	else if (Key == EKeys::G)
	{
		Upscaler.NextFrameGeneration();
		Settings.FrameGeneration = Upscaler.GetFrameGeneration();
	}
	else
	{
		bHelp = !bHelp;
		return true;
	}
	ApplySettings();   // (in use already)
	SaveProfile();
	return true;
}
