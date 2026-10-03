// The sprites of the original.
#pragma once

#include "CoreMinimal.h"

/**
 * The sprites of the original, extracted to assets/ (sprites.json, sprites/NNN.png): their sizes, so a figure is as
 * big as its sprite, and their pixels, for the drawing of a level (its tiles are sprites) and the speech bubbles.
 */
class FUghSprites
{
public:
	/** Reads sprites.json in `AssetsDir`; false and the reason when it cannot. */
	bool Load(const FString& AssetsDir, FString& OutError);

	/** The size of `Sprite` in pixels; DefaultSize for none (-1) or an unknown one. */
	FIntPoint Size(int32 Sprite) const;

	/** The pixels of `Sprite`, row by row (Size wide), colour 0 of the original transparent; empty for none. */
	const TArray<FColor>& Pixels(int32 Sprite) const;

private:
	static constexpr int32 DefaultSize = 16;

	struct FSprite
	{
		FIntPoint Size = FIntPoint::ZeroValue;
		FString File;   // the PNG, read the first time its pixels are asked for
	};

	FString Dir;
	TArray<FSprite> Sprites;   // by sprite index
	mutable TMap<int32, TArray<FColor>> Loaded;
};
