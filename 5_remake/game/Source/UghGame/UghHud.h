// What the original showed in text.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UghHud.generated.h"

/**
 * The text of the game: the status line (level, lives, score, multiplier, energy), the level caption, the end of the
 * game, a missing data file, and the keys with the upscaler under the status line.
 */
UCLASS()
class AUghHud : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawCentred(const FString& Text, float Y, const FLinearColor& Color, float Scale);
};
