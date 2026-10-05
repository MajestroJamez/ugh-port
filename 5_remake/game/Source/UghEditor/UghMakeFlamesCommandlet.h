// Bakes the flames of the campfires and torches.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UghMakeFlamesCommandlet.generated.h"

/**
 * Simulates the flames (FUghFireSim) and writes their flipbooks (UghFlipbook, UghFlames) as textures to
 * Content/Generated, and as pictures to Saved/Flames/<name>.png for a look; build.ps1 runs it after UghMakeAssets:
 * UnrealEditor-Cmd UghGame.uproject -run=UghMakeFlames. The same every time (the simulation is seeded).
 */
UCLASS()
class UUghMakeFlamesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUghMakeFlamesCommandlet();
	virtual int32 Main(const FString& Params) override;
};
