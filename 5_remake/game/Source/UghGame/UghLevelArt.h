// The original's drawing of a level.
#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class FUghSprites;
class UTexture2D;

/**
 * The original's drawing of every level: its 20 x 16 tiles (assets/levels.json, the levels in the order of the game
 * data, so a level's index is its level_id), each a sprite drawn 16 x 12 px apart with colour 0 transparent onto a
 * black screen - as the original's level setup draws them. The rock and the cave take their colours from it.
 */
class FUghLevelArt
{
public:
	/** Takes the tiles of the levels' file (UghJson::LevelsFile) read from `Path`; false and the reason when not. */
	bool Load(const FJsonObject& LevelsFile, const FString& Path, FString& OutError);

	/** The drawing of level `LevelId` as a texture of the screen's size (black when the level is unknown). */
	UTexture2D* Draw(UObject* Outer, int32 LevelId, const FUghSprites& Sprites) const;

private:
	static constexpr int32 TileWidth = 16, TileHeight = 12;

	TArray<TArray<int32>> Tiles;   // by level_id: the tiles row by row
	int32 Columns = 0;
};
