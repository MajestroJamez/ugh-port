#include "UghPlayerController.h"

#include "Engine/World.h"
#include "UghGameMode.h"

void AUghPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

bool AUghPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	AUghGameMode* Mode = GetWorld()->GetAuthGameMode<AUghGameMode>();
	if (Mode && Mode->HandleKey(Params.Key, Params.Event))
	{
		return true;
	}
	return Super::InputKey(Params);
}
