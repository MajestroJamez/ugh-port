// The game: the menu, the logic, its keys and its diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UghCameraLog.h"
#include "UghControls.h"
#include "UghDecorations.h"
#include "UghDunk.h"
#include "UghFigureActions.h"
#include "UghFling.h"
#include "UghGhost.h"
#include "UghImpacts.h"
#include "UghIntro.h"
#include "UghLevelArt.h"
#include "UghLively.h"
#include "UghMenu.h"
#include "UghMotionBlur.h"
#include "UghPads.h"
#include "UghPasswords.h"
#include "UghProfile.h"
#include "UghReplays.h"
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
class AUghGhosts;
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
 * volumes) and saved whenever it changes. The logic records each level played (FUghSimulation::TakeEndedLevel): the
 * best of each level and mode is kept (FUghReplays, the folder Replays next to the profile), F5 saves the last level's
 * (in its next caption, on the card of the game's end); a replay chosen in the menu (its screen Replays, R on a stone of
 * the level selection) is watched as the game it was (FUghSimulation::Watch: Esc stops it); the best flies as a ghost
 * beside the player (FUghGhost: a logic of its own in step with the play, AUghGhosts). Keys of the frontend: in a
 * game U the next upscaler, G the frame generation, F1 (a gamepad's Y) the help of the keys, F5 the last level's replay
 * saved; everywhere Page Up and Page Down the volume.
 *
 * -UghAssets=<folder> reads the data from elsewhere than assets/ (of the package, else of the repository).
 * -UghProfile=<file> keeps the profile elsewhere than Saved/UghProfile.json; -UghReplays=<folder> the replays elsewhere
 * than the folder Replays next to it (the autopilot reads replays only from there, and never writes).
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
	/** The passengers on land waving, ducking, glad (AUghFigures). */
	const FUghLively& GetLively() const;
	/** The copters falling into the sea. */
	const FUghDunks& GetDunks() const { return Dunks; }
	/** The help of the keys is wanted (F1 or a gamepad's Y in a game). */
	bool IsHelpWanted() const { return bHelp; }
	/** The folder of the game's data; empty before the game starts. */
	const FString& GetAssets() const { return Assets; }
	/** Why there is no game (the data cannot be read); empty when there is one. */
	const FString& GetProblem() const { return Problem; }

	/** The replays kept (the screen Replays, the best of each level). */
	const FUghReplays& GetReplays() const { return Replays; }
	/** A replay is watched (not a game played). */
	bool IsWatching() const { return !bInMenu && Simulation.IsWatching(); }
	const TSharedPtr<FUghReplay>& GetWatched() const { return Watched; }
	/** The ghost of the best run, and its copters shown. */
	const FUghGhost& GetGhost() const { return Ghost; }
	const AUghGhosts* GetGhosts() const { return Ghosts; }
	/**
	 * The replay of the last level that ended in the game played (or just over): its caption after it, the card of the
	 * game's end offer to save it (F5). Whether it became the best of its level, the file it was saved to (empty: not).
	 */
	const TSharedPtr<FUghReplay>& GetLastLevel() const { return LastLevel; }
	bool IsLastLevelBest() const { return bLastLevelBest; }
	const FString& GetLastLevelSaved() const { return LastLevelSaved; }
	/** What was last done with a replay (saved, not saved), and how many such notices there were. */
	const FString& GetReplayNotice() const { return ReplayNotice; }
	int32 GetReplayNoticeCount() const { return ReplayNoticeCount; }
	/** Watches a replay (from the menu, the autopilot): false when the logic refuses it. */
	bool StartWatching(const TSharedPtr<FUghReplay>& Replay);

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
	/** U, G, F1 (a gamepad's Y), F5 (the last level's replay saved): true when it was one of them. */
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
	/**
	 * Back to the menu after a game: how it ended, the level of the menu's choice behind it; after a replay watched the
	 * screen it was chosen on.
	 */
	void OpenMenu();
	/**
	 * A level ended in the game played (UghGameModeReplays.cpp): its replay labelled (the password, the profile's name,
	 * now), kept as the best of its level and mode if it is better, the last level's (F5 saves it).
	 */
	void OnLevelEnded(const TSharedPtr<FUghReplay>& Ended);
	/** F5: the last level's replay saved into the replays' folder. */
	void SaveLastLevel();
	void SayReplay(const FString& Text);
	/**
	 * The ghost of the best run (Settings > Ghost): the replay of the level played (its best, of the same logic; the
	 * autopilot's -UghShotGhost), none in the menu or while a replay is watched; in step with the game's play.
	 */
	void UpdateGhost();
	/**
	 * The ambience (FUghAmbience): the mood, the place (the menu's stone, the archipelago, the flight, the play), as much
	 * as the picture `Shown`, the camera's ears, the fires burning over the water's `Surface`, the lianas rustling.
	 */
	void Hear(const ugh_logic_view& View, double Surface, double Shown, bool bMenuView, bool bIsles,
		const FUghCameraPose& Pose);
	/** The level's campfires and torches among its `Decorations`, heard from now on. */
	void HearFires(const TArray<FUghDecoration>& Decorations);
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
	FUghReplays Replays;
	TSharedPtr<FUghReplay> Watched;     // the replay watched
	FUghGhost Ghost;                    // of the best run of the level played
	TSharedPtr<FUghReplay> ShotGhost;   // -UghShotGhost's replay
	TSharedPtr<FUghReplay> LastLevel;   // the last level that ended in the game played
	bool bLastLevelBest = false;
	FString LastLevelSaved;
	FString ReplayNotice;
	int32 ReplayNoticeCount = 0;
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
	double CollectedAt = -1e9;  // when the garbage was last collected in the black (FPlatformTime)
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
	TArray<FUghDecoration> HeardFires;   // the level's campfires and torches (Hear)

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
	UPROPERTY() TObjectPtr<AUghGhosts> Ghosts;
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
