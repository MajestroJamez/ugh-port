// The menu and the HUD on the screen.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UghHud.generated.h"

class AUghGameMode;
class SWidget;
class UTexture2D;
struct FUghUiState;

/**
 * Puts the screen of UghUi (SUghScreen: the menu, the status of the play, the captions, the help, the end of a game)
 * into the game's viewport once the game mode has started (its fonts are in the game's data, its pictures carved by
 * UghStoneArt), and every frame takes what it shows from the game (FUghUiState): the menu, the logic's view, the
 * scores earned projected where they were earned (AUghEffects), when the help shows (F1, and at the first level),
 * a setting just changed (the volume, the upscaler).
 */
UCLASS()
class AUghHud : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** At the first level the help shows through its caption and this long into the play (seconds). */
	static constexpr double FirstHelpSeconds = 6;

private:
	void Build(const AUghGameMode& Mode);
	void Update(const AUghGameMode& Mode, double Seconds);
	/** How much the help shows now, `Seconds` after the last frame. */
	void UpdateHelp(const AUghGameMode& Mode, double Seconds);
	void UpdateNotice(const AUghGameMode& Mode, double Seconds);

	TSharedPtr<FUghUiState> State;
	TSharedPtr<SWidget> Screen;
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> Pictures;
	double PlaySeconds = 0;       // since the play of the level began
	bool bFirstHelpDone = false;  // in this game
	bool bLastHelpWanted = false; // F1 of the last frame
	int32 LastVolume = -1;
	FString LastUpscaler;
};
