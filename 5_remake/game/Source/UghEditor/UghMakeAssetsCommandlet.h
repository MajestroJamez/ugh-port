// Makes the materials of the diorama.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "UghMakeAssetsCommandlet.generated.h"

class UMaterial;

/**
 * Writes the materials of UghMaterials.h to Content/Generated (assets as code: nothing binary in git; the HLSL of the
 * custom nodes is in Shaders/). build.ps1 runs it after every build: UnrealEditor-Cmd UghGame.uproject
 * -run=UghMakeAssets. Each material's nodes are made anew.
 */
UCLASS()
class UUghMakeAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UUghMakeAssetsCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/** Each makes its material's nodes; false when something it needs is missing (its shader code). */
	static bool MakeClay(UMaterial* Material);
	static bool MakeRock(UMaterial* Material);
	static bool MakeCliff(UMaterial* Material);
	static bool MakeTurf(UMaterial* Material);
	static bool MakeSprite(UMaterial* Material);
	static bool MakeGhost(UMaterial* Material);
	static bool MakePbr(UMaterial* Material);
	static bool MakeScan(UMaterial* Material);
	static bool MakeSky(UMaterial* Material);
	/** The water and the weather (UghMakeWeather.cpp): the water, the rain's streaks, its splashes, a raindrop. */
	static bool MakeWater(UMaterial* Material);
	static bool MakeRain(UMaterial* Material);
	static bool MakeSplash(UMaterial* Material);
	static bool MakeRaindrop(UMaterial* Material);
	/** The springs' water (UghMakeFlow.cpp): flowing water, a waterfall's mist. */
	static bool MakeFlow(UMaterial* Material);
	static bool MakeMist(UMaterial* Material);
	/** The fires (UghMakeFire.cpp): a flame of a flipbook, its sparks, its smoke, burning wood. */
	static bool MakeFlame(UMaterial* Material);
	static bool MakeSparks(UMaterial* Material);
	static bool MakeSmoke(UMaterial* Material);
	static bool MakeEmbers(UMaterial* Material);
	/** The figures (UghMakeFigures.cpp): the blower's snort of dust, the flyer's wings, the figures' halo. */
	static bool MakePuff(UMaterial* Material);
	static bool MakeMembrane(UMaterial* Material);
	static bool MakeFigureHalo(UMaterial* Material);
	/** The bursts of the events (UghMakeEffects.cpp): dust, smoke and spray; tumbling bits; fire, sparks and glints. */
	static bool MakeBurst(UMaterial* Material);
	static bool MakeBits(UMaterial* Material);
	static bool MakeGlint(UMaterial* Material);
};
