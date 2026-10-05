// The game: the menu, the logic, its keys and its diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UghFigureActions.h"
#include "UghIntro.h"
#include "UghLevelArt.h"
#include "UghMenu.h"
#include "UghPasswords.h"
#include "UghShot.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghUpscaler.h"
#include "UghGameMode.generated.h"

class AUghBackground;
class AUghCampfire;
class AUghTorches;
class AUghCliffDressing;
class AUghCopters;
class AUghEffects;
class AUghFalls;
class AUghFigures;
class AUghRain;
class AUghScenery;
class AUghSigns;
class AUghSpeaker;
class AUghStage;
class AUghSeaStack;
class AUghWater;

/**
 * The remake: the menu (FUghMenu) starts a game, the logic runs at its own tick (FUghSimulation) with the keys
 * (FUghKeyboard), each frame is shown between two of its steps in the diorama (AUghStage, AUghBackground, AUghSeaStack,
 * AUghSigns, AUghWater, AUghFalls, AUghRain, AUghCopters, AUghFigures, AUghCampfire, AUghTorches, AUghScenery,
 * AUghCliffDressing, the screen of AUghHud), heard (AUghSpeaker) and its events seen as bursts (AUghEffects); the end
 * of a game goes back to the menu, which shows how it ended. Each level has its mood (UghMood); its first caption shows
 * the camera flying over the sea to the stone the level is carved into (FUghIntro).
 * Behind the menu the camera swings slowly around that stone (UghMenuView), the level the menu would start carved into
 * it. No map: the scene is built here. Keys of the frontend: in a game U the next upscaler, G the frame generation, F1
 * the help of the keys; everywhere Page Up and Page Down the volume.
 *
 * -UghAssets=<folder> reads the data from elsewhere than assets/ (of the package, else of the repository).
 * -UghShot=<folder>: the game plays by itself for screenshots (FUghShot). -UghNoIntro: a level starts without the
 * flight to the stone (FUghIntro, AUghSeaStack).
 */
UCLASS()
class AUghGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUghGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** A key event from the player controller (or the autopilot of FUghShot); true when the game used it. */
	bool HandleKey(const FKey& Key, EInputEvent Event);

	const FUghSimulation& GetSimulation() const { return Simulation; }
	/** The bursts of the events (the HUD draws their scores). */
	const AUghEffects* GetEffects() const { return Effects; }
	const FUghUpscaler& GetUpscaler() const { return Upscaler; }
	const FUghPasswords& GetPasswords() const { return Passwords; }
	/** The volume of the sounds in percent. */
	int32 GetVolumePercent() const;
	/** The menu is shown (no game is played). */
	bool IsInMenu() const { return bInMenu; }
	const FUghMenu& GetMenu() const { return Menu; }
	/** The flight to the stone at the start of a level. */
	const FUghIntro& GetIntro() const { return Intro; }
	/** The help of the keys is wanted (F1 in a game). */
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
	void HandleMenuKey(const FKey& Key);
	/** Page Up and Page Down: the volume; true when it was one of them. */
	bool HandleVolumeKey(const FKey& Key, EInputEvent Event);
	/** The sounds and the effects of the logic's events and of the frame's view (UghEvents). */
	void PlayEvents();
	/** A shot of a burst (-UghShotEffect): held by the first copter in the play, framed. */
	void HoldShotEffect(const ugh_logic_view& View);
	/** Back to the menu after a game: how it ended, the level of the menu's choice behind it. */
	void OpenMenu();
	void Quit();

	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghFigureActions FigureActions;
	FUghLevelArt LevelArt;
	FUghPasswords Passwords;
	FUghMenu Menu{ Passwords };
	FUghUpscaler Upscaler;
	FUghShot Shot;
	bool bShooting = false;   // -UghShot
	TOptional<FVector2D> ShotLook;   // the middle of what the shot looks at (FUghShot::GetLook, GetEffect), pixels
	double ShotAround = FUghShot::LookAround;   // and how far around it
	bool bInMenu = true;
	FUghGameChoice Previewed;   // whose level the diorama shows behind the menu
	FKey StartKey;              // the key that started the game: its release is not a key of the game
	double MenuTime = 0;        // seconds of the menu's camera (UghMenuView)
	bool bHelp = false;         // F1
	FString Assets;
	FString Problem;
	FUghIntro Intro;
	bool bIntro = false;        // the levels start with the flight (FUghIntro::bFlies, not -UghNoIntro)
	int32 IntroLevel = -1;      // the level (of the mode) of the last flight in this game
	bool bIntroScene = false;   // since its flight began until the play is fully shown: the scene is not black

	UPROPERTY() TObjectPtr<AUghStage> Stage;
	UPROPERTY() TObjectPtr<AUghBackground> Background;
	UPROPERTY() TObjectPtr<AUghSigns> Signs;
	UPROPERTY() TObjectPtr<AUghWater> Water;
	UPROPERTY() TObjectPtr<AUghFalls> Falls;
	UPROPERTY() TObjectPtr<AUghSeaStack> SeaStack;
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
