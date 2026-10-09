// The game: the menu, the logic, its keys and its diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UghCameraLog.h"
#include "UghControls.h"
#include "UghDunk.h"
#include "UghFigureActions.h"
#include "UghFling.h"
#include "UghImpacts.h"
#include "UghIntro.h"
#include "UghLevelArt.h"
#include "UghMenu.h"
#include "UghMotionBlur.h"
#include "UghPads.h"
#include "UghPasswords.h"
#include "UghProfile.h"
#include "UghShot.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghUpscaler.h"
#include "UghGameMode.generated.h"

class AUghArchipelago;
class AUghBackground;
class AUghCampfire;
class AUghTorches;
class AUghCliffDressing;
class AUghCopters;
class AUghEffects;
class AUghFalls;
class AUghFigures;
class AUghFringe;
class AUghRain;
class AUghScenery;
class AUghSigns;
class AUghSpeaker;
class AUghStage;
class AUghSeaStack;
class AUghWater;

/**
 * The remake: the menu (FUghMenu) starts a game, the logic runs at its own tick (FUghSimulation) with the keys and the
 * gamepads (FUghControls), each frame is shown between two of its steps in the diorama (AUghStage, AUghBackground,
 * AUghSeaStack, AUghFringe, AUghSigns, AUghWater, AUghFalls, AUghRain, AUghCopters, AUghFigures, AUghCampfire,
 * AUghTorches, AUghScenery, AUghCliffDressing, the screen of AUghHud), heard (AUghSpeaker) and its events seen as bursts
 * (AUghEffects); the end of a game goes back to the menu, which shows how it ended (and takes a high score's name).
 * Each level has its mood (UghMood); its first caption shows the camera flying over the sea to the stone the level is
 * carved into (FUghIntro). Behind the menu the camera swings slowly around that stone (UghMenuView), the level the menu
 * would start carved into it; PLAY opens the level selection (FUghIsles): the camera flies over an archipelago of the
 * mode's levels (AUghArchipelago), a stone is chosen and flown to, its level starts. No map: the scene is built here.
 * The profile (FUghProfile: the settings, the high scores, the levels the last games got to and done) is read at the start, put into the engine (UghGraphics, FUghUpscaler, the sounds'
 * volumes) and saved whenever it changes. Keys of the frontend: in a game U the next upscaler, G the frame generation,
 * F1 (a gamepad's Y) the help of the keys; everywhere Page Up and Page Down the volume.
 *
 * -UghAssets=<folder> reads the data from elsewhere than assets/ (of the package, else of the repository).
 * -UghProfile=<file> keeps the profile elsewhere than Saved/UghProfile.json.
 * -UghShot=<folder>: the game plays by itself for screenshots (FUghShot) with the default profile (or the one of
 * -UghProfile), which it never saves, and the window as it is; PLAY starts the game at once (no level selection) but
 * for a shot of it (FUghShot::WantsIsles). -UghNoIntro: a level starts without the flight to the stone (FUghIntro,
 * AUghSeaStack). -UghNoIsles: PLAY starts the game at once, without the level selection (FUghIsles, AUghArchipelago).
 */
UCLASS()
class AUghGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUghGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/**
	 * A key event from the player controller (a key of the keyboard or a gamepad's button from `Device`) or the
	 * autopilot of FUghShot; true when the game used it.
	 */
	bool HandleKey(const FKey& Key, EInputEvent Event, FInputDeviceId Device = FInputDeviceId());

	const FUghSimulation& GetSimulation() const { return Simulation; }
	/** The bursts of the events (the HUD draws their scores). */
	const AUghEffects* GetEffects() const { return Effects; }
	const FUghUpscaler& GetUpscaler() const { return Upscaler; }
	const FUghPasswords& GetPasswords() const { return Passwords; }
	/** The settings, the high scores (the menu, the HUD). */
	const FUghProfile& GetProfile() const { return Profile; }
	/** What the graphics can be set to on this computer. */
	const FUghDisplayOptions& GetDisplayOptions() const { return DisplayOptions; }
	/** The menu is shown (no game is played). */
	bool IsInMenu() const { return bInMenu; }
	const FUghMenu& GetMenu() const { return Menu; }
	/** The flight to the stone at the start of a level. */
	const FUghIntro& GetIntro() const { return Intro; }
	/** The passengers knocked off their pads, flung into the sea. */
	const FUghFlings& GetFlings() const { return Flings; }
	/** The copters falling into the sea. */
	const FUghDunks& GetDunks() const { return Dunks; }
	/** The help of the keys is wanted (F1 or a gamepad's Y in a game). */
	bool IsHelpWanted() const { return bHelp; }
	/** The folder of the game's data; empty before the game starts. */
	const FString& GetAssets() const { return Assets; }
	/** Why there is no game (the data cannot be read); empty when there is one. */
	const FString& GetProblem() const { return Problem; }

private:
	void BuildStage();
	/** The frame of the view (`Seconds` after the last one). */
	void ShowFrame(double Seconds);
	void BuildLevel(const ugh_logic_view& View);
	/**
	 * The flight to the stone (FUghIntro) at the first caption of a level: starts it, flies it on `Seconds`, hurries it
	 * when the play begins, shows the stone meanwhile.
	 */
	void FlyIntro(const ugh_logic_view& View, double Seconds);
	/** A key pressed in the menu (a gamepad's button as FUghControls::MenuKeyOf gives it). */
	void HandleMenuKey(const FKey& Key);
	/** U, G, F1 (a gamepad's Y): true when it was one of them. */
	bool HandleFrontendKey(const FKey& Key, EInputEvent Event);
	/** Page Up and Page Down: the volume; true when it was one of them. */
	bool HandleVolumeKey(const FKey& Key, EInputEvent Event);
	/** The sounds and the effects of the logic's events and of the frame's view (UghEvents). */
	void PlayEvents();
	/** A shot of a burst (-UghShotEffect): held by the first copter in the play, framed. */
	void HoldShotEffect(const ugh_logic_view& View);
	/** The profile of the file (FUghProfile::DefaultPath; the autopilot's: only of -UghProfile), the display's options. */
	void LoadProfile();
	/** Puts the settings into the engine (what changed since the last time). */
	void ApplySettings();
	/** Saves the profile (not the autopilot). */
	void SaveProfile();
	/**
	 * A level the game got to by playing (not where it started): the last one of the mode in the profile; the level it
	 * went on from done (the level selection's green).
	 */
	void NoteLevel(const ugh_logic_view& View);
	/** Back to the menu after a game: how it ended, the level of the menu's choice behind it. */
	void OpenMenu();
	/** A new game as chosen (the menu's PLAY, the level selection's stone); `Key` started it (its release is no key). */
	void StartGame(const FUghGameChoice& Choice, const FKey& Key);
	void Quit();

	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghFigureActions FigureActions;
	FUghFlings Flings;
	FUghDunks Dunks;
	FUghMotionBlur MotionBlur;   // of the play: the copters blur the more the faster they fly
	FUghImpacts Impacts;         // a crash, a stone on an enemy, a fall into the sea, a bump: the camera shaken, a pad rumbling
	FUghPads Pads;
	FUghLevelArt LevelArt;
	FUghPasswords Passwords;
	FUghProfile Profile;
	FString ProfilePath;
	TOptional<FUghSettings> Applied;   // the settings put into the engine last
	FUghDisplayOptions DisplayOptions;
	FUghMenu Menu{ Passwords, Profile, DisplayOptions };
	FUghControls Controls{ Profile.Settings.Keys };
	FUghGameChoice Playing;    // what the game being played started with
	FUghUpscaler Upscaler;
	FUghShot Shot;
	FUghCameraLog CameraLog;   // -UghCameraLog
	bool bShooting = false;   // -UghShot
	TOptional<FVector2D> ShotLook;   // the middle of what the shot looks at (FUghShot::GetLook, GetEffect), pixels
	double ShotAround = FUghShot::LookAround;   // and how far around it
	bool bInMenu = true;
	FUghGameChoice Previewed;   // whose level the diorama shows behind the menu
	FKey StartKey;              // the key that started the game: its release is not a key of the game
	double MenuTime = 0;        // seconds of the menu's camera (UghMenuView)
	bool bHelp = false;         // F1, a gamepad's Y
	FString Assets;
	FString Problem;
	FUghIntro Intro;
	bool bIntro = false;        // the levels may start with the flight (FUghIntro::bFlies, not -UghNoIntro)
	int32 IntroLevel = -1;      // the level (of the mode) of the last flight in this game
	bool bIntroScene = false;   // since its flight began until the play is fully shown: the scene is not black
	FUghCameraPose CameraPose;  // the camera of the last frame
	double SeaZ = 0;            // the sea's surface of the last frame (the world)
	int32 PlayedLevel = -1;     // the level of the mode being played (when the game goes on from it, it is done)

	UPROPERTY() TObjectPtr<AUghStage> Stage;
	UPROPERTY() TObjectPtr<AUghBackground> Background;
	UPROPERTY() TObjectPtr<AUghSigns> Signs;
	UPROPERTY() TObjectPtr<AUghWater> Water;
	UPROPERTY() TObjectPtr<AUghFalls> Falls;
	UPROPERTY() TObjectPtr<AUghSeaStack> SeaStack;
	UPROPERTY() TObjectPtr<AUghArchipelago> Archipelago;
	UPROPERTY() TObjectPtr<AUghFringe> Fringe;
	UPROPERTY() TObjectPtr<AUghRain> Rain;
	UPROPERTY() TObjectPtr<AUghCopters> Copters;
	UPROPERTY() TObjectPtr<AUghFigures> Figures;
	UPROPERTY() TObjectPtr<AUghEffects> Effects;
	UPROPERTY() TObjectPtr<AUghCampfire> Campfire;
	UPROPERTY() TObjectPtr<AUghTorches> Torches;
	UPROPERTY() TObjectPtr<AUghScenery> Scenery;
	UPROPERTY() TObjectPtr<AUghCliffDressing> Dressing;
	UPROPERTY() TObjectPtr<AUghSpeaker> Speaker;
	int32 BackgroundLevel = -1;   // the level_id the background shows
	int32 MoodLevel = -1;         // the level (of the mode) whose mood the stage shows
};
