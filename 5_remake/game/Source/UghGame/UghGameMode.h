// The game: the logic, its keys and its grey boxes.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UghSimulation.h"
#include "UghSpriteSizes.h"
#include "UghUpscaler.h"
#include "UghGameMode.generated.h"

class ACameraActor;
class AUghBackground;
class AUghFigures;

/**
 * The remake in grey boxes: runs the logic at its own tick (FUghSimulation), passes it the keys (FUghKeyboard) and
 * shows each frame between two of its steps (AUghBackground, AUghFigures, the HUD). No map: the scene is built here.
 * Keys of the frontend: U the next upscaler, G the frame generation; when the game is over Enter starts a new one
 * and Esc quits.
 *
 * -UghAssets=<folder> reads the data from elsewhere than the repository's assets/. -UghShot[=<file.png>] plays by
 * itself (it skips the caption, lifts off) and saves one screenshot of level 1, then quits: a check without a window
 * (with -RenderOffscreen).
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
	void FitCamera();
	void TickShot(float DeltaSeconds);
	void Quit();

	FUghSimulation Simulation;
	FUghSpriteSizes SpriteSizes;
	FUghUpscaler Upscaler;
	FString Problem;

	UPROPERTY() TObjectPtr<AUghBackground> Background;
	UPROPERTY() TObjectPtr<AUghFigures> Figures;
	UPROPERTY() TObjectPtr<ACameraActor> Camera;
	int32 BackgroundLevel = -1;   // the level_id the background shows

	/** -UghShot: where the screenshot goes (empty: no shot); the phase it is in, since when; how long it runs. */
	FString ShotPath;
	int32 ShotPhase = -1;
	double ShotPhaseTime = 0;
	double ShotTotalTime = 0;
};
