// The game: the logic, its keys and its diorama.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UghLevelArt.h"
#include "UghShot.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghUpscaler.h"
#include "UghGameMode.generated.h"

class AUghBackground;
class AUghCampfire;
class AUghFigures;
class AUghStage;

/**
 * The remake: runs the logic at its own tick (FUghSimulation), passes it the keys (FUghKeyboard) and shows each frame
 * between two of its steps in the diorama (AUghStage, AUghBackground, AUghFigures, AUghCampfire, the HUD). No map: the
 * scene is built here. Keys of the frontend: U the next upscaler, G the frame generation; when the game is over Enter
 * starts a new one and Esc quits.
 *
 * -UghAssets=<folder> reads the data from elsewhere than assets/ (of the package, else of the repository).
 * -UghShot: the game plays by itself for a screenshot (FUghShot).
 */
UCLASS()
class AUghGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUghGameMode();
	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** A key event from the player controller; true when the game used it. */
	bool HandleKey(const FKey& Key, EInputEvent Event);
	/** The keys of the frontend (HandleKey), for the HUD: always, and when the game is over. */
	static const TCHAR* KeysHelp() { return TEXT("U upscaler, G frame generation"); }
	static const TCHAR* GameOverKeysHelp() { return TEXT("Enter: a new game   Esc: quit"); }

	const FUghSimulation& GetSimulation() const { return Simulation; }
	const FUghUpscaler& GetUpscaler() const { return Upscaler; }
	/** Why there is no game (the data cannot be read); empty when there is one. */
	const FString& GetProblem() const { return Problem; }

private:
	void BuildStage();
	void ShowFrame();
	void BuildLevel(const ugh_logic_view& View);
	void Quit();

	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghLevelArt LevelArt;
	FUghUpscaler Upscaler;
	FUghShot Shot;
	bool bShooting = false;   // -UghShot
	FString Problem;

	UPROPERTY() TObjectPtr<AUghStage> Stage;
	UPROPERTY() TObjectPtr<AUghBackground> Background;
	UPROPERTY() TObjectPtr<AUghFigures> Figures;
	UPROPERTY() TObjectPtr<AUghCampfire> Campfire;
	int32 BackgroundLevel = -1;   // the level_id the background shows
};
