// Makes the materials of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UghMakeAssetsCommandlet.generated.h"

class UMaterial;

/**
 * Writes the materials of UghMaterials.h to Content/Generated (assets as code: nothing binary in git). build.ps1 runs
 * it after every build: UnrealEditor-Cmd UghGame.uproject -run=UghMakeAssets. Each material's nodes are made anew.
 */
UCLASS()
class UUghMakeAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUghMakeAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	static void MakeClay(UMaterial* Material);
	static void MakeRock(UMaterial* Material);
	static void MakeWater(UMaterial* Material);
	static void MakeFire(UMaterial* Material);
	static void MakeSprite(UMaterial* Material);
	static void MakePbr(UMaterial* Material);
};
