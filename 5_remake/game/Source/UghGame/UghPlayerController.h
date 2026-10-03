// The player's keys.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UghPlayerController.generated.h"

/** Passes every key event to the game mode (no input actions: the logic wants the raw presses and releases). */
UCLASS()
class AUghPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;

protected:
	virtual void BeginPlay() override;
};
