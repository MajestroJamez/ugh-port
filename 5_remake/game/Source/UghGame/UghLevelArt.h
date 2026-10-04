// The original's drawing of a level.
#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class FUghSprites;

/** A tile of a level's drawing: its sprite and its top left corner on the screen (pixels). */
struct FUghArtTile
{
	int32 Sprite = 0;
	FIntPoint At = FIntPoint::ZeroValue;
};

/**
 * The original's drawing of every level: its 20 x 16 tiles (assets/levels.json, the levels in the order of the game
 * data, so a level's index is its level_id), each a sprite drawn 16 x 12 px apart with colour 0 transparent onto a
 * black screen - as the original's level setup draws them. The rock takes its colours from it; the boards with the
 * pads' numbers are shown as they are drawn (Signs).
 */
class FUghLevelArt
{
public:
	/** Takes the tiles of the levels' file (UghJson::LevelsFile) read from `Path`; false and the reason when not. */
	bool Load(const FJsonObject& LevelsFile, const FString& Path, FString& OutError);

	/** The drawing of level `LevelId`: the screen's pixels row by row (black when the level is unknown). */
	TArray<FColor> Draw(int32 LevelId, const FUghSprites& Sprites) const;

	/** The tiles of level `LevelId` that are boards with a pad's number (or a blank one). */
	TArray<FUghArtTile> Signs(int32 LevelId) const;

private:
	static constexpr int32 TileWidth = 16, TileHeight = 12;
	/** The sprites of the boards: I, II, III, IIII, V and a blank one. */
	static constexpr int32 FirstSign = 85, LastSign = 90;

	TArray<TArray<int32>> Tiles;   // by level_id: the tiles row by row
	int32 Columns = 0;
};
