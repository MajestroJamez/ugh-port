// What the original showed in text.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UghHud.generated.h"

class AUghGameMode;

/**
 * The text of the game: the menu (FUghMenu) with how the last game ended; in a game the status line (level, lives,
 * score, multiplier, energy), the keys with the upscaler under it and the level caption; a missing data file.
 */
UCLASS()
class AUghHud : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawMenu(const AUghGameMode& Mode);
	void DrawCentred(const FString& Text, float Y, const FLinearColor& Color, float Scale);
};
