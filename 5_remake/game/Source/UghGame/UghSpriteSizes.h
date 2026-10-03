// How big the sprites of the original are.
#pragma once

#include "CoreMinimal.h"

/** The size of every sprite of the original (assets/sprites.json), so a figure's box is as big as its sprite. */
class FUghSpriteSizes
{
public:
	/** Reads the sizes; false and the reason when it cannot. */
	bool Load(const FString& Path, FString& OutError);

	/** The size of `Sprite` in pixels; DefaultSize for none (-1) or an unknown one. */
	FIntPoint Size(int32 Sprite) const;

private:
	static constexpr int32 DefaultSize = 16;

	TArray<FIntPoint> Sizes;   // by sprite index
};
