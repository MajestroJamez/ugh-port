// The player's keys.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UghPlayerController.generated.h"

/**
 * Passes every key event - of the keyboard, of every gamepad (Config/Windows/WindowsInput.ini gives them all to the one
 * player) - to the game mode with its device (no input actions: the logic wants the raw presses and releases).
 */
UCLASS()
class AUghPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

protected:
	virtual void BeginPlay() override;
};
