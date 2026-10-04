// The game: the menu, the logic, its keys and its diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UghFigureActions.h"
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
class AUghCliffDressing;
class AUghCopters;
class AUghFigures;
class AUghScenery;
class AUghSpeaker;
class AUghStage;

/**
 * The remake: the menu (FUghMenu) starts a game, the logic runs at its own tick (FUghSimulation) with the keys
 * (FUghKeyboard), each frame is shown between two of its steps in the diorama (AUghStage, AUghBackground,
 * AUghCopters, AUghFigures, AUghCampfire, AUghScenery, AUghCliffDressing, the HUD) and heard (AUghSpeaker); the end of
 * a game goes back to the menu.
 * Behind the menu the diorama shows the level the menu would start, dimmed. No map: the scene is built here. Keys of
 * the frontend: in a game U the next upscaler, G the frame generation; everywhere Page Up and Page Down the volume.
 *
 * -UghAssets=<folder> reads the data from elsewhere than assets/ (of the package, else of the repository).
 * -UghShot=<folder>: the game plays by itself for screenshots (FUghShot).
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
	/** The keys of the frontend (HandleKey), for the HUD. */
	static const TCHAR* KeysHelp() { return TEXT("U upscaler, G frame generation, PgUp/PgDn volume"); }

	const FUghSimulation& GetSimulation() const { return Simulation; }
	const FUghUpscaler& GetUpscaler() const { return Upscaler; }
	const FUghPasswords& GetPasswords() const { return Passwords; }
	/** The volume of the sounds in percent. */
	int32 GetVolumePercent() const;
	/** The menu is shown (no game is played). */
	bool IsInMenu() const { return bInMenu; }
	const FUghMenu& GetMenu() const { return Menu; }
	/** How the last game ended, for the menu; empty before the first one. */
	const FString& GetLastGame() const { return LastGame; }
	/** Why there is no game (the data cannot be read); empty when there is one. */
	const FString& GetProblem() const { return Problem; }

private:
	/** How much of the level the menu lets through, 0 .. 1. */
	static constexpr double MenuShown = 0.45;

	void BuildStage();
	/** The frame of the view (`Seconds` after the last one). */
	void ShowFrame(double Seconds);
	void BuildLevel(const ugh_logic_view& View);
	void HandleMenuKey(const FKey& Key);
	/** Page Up and Page Down: the volume; true when it was one of them. */
	bool HandleVolumeKey(const FKey& Key, EInputEvent Event);
	/** The sounds of the logic's events and of the frame's view. */
	void PlaySounds();
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
	bool bInMenu = true;
	FUghGameChoice Previewed;   // whose level the diorama shows behind the menu
	FKey StartKey;              // the key that started the game: its release is not a key of the game
	FString LastGame;
	FString Problem;

	UPROPERTY() TObjectPtr<AUghStage> Stage;
	UPROPERTY() TObjectPtr<AUghBackground> Background;
	UPROPERTY() TObjectPtr<AUghCopters> Copters;
	UPROPERTY() TObjectPtr<AUghFigures> Figures;
	UPROPERTY() TObjectPtr<AUghCampfire> Campfire;
	UPROPERTY() TObjectPtr<AUghScenery> Scenery;
	UPROPERTY() TObjectPtr<AUghCliffDressing> Dressing;
	UPROPERTY() TObjectPtr<AUghSpeaker> Speaker;
	int32 BackgroundLevel = -1;   // the level_id the background shows
};
