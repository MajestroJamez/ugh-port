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
 * black screen - as the original's level setup draws them. The rock takes its colours from it; its boards with the
 * pads' numbers stand where the pads get theirs (Signs, UghPadSigns); its doors are the cave's entrances (Doors,
 * UghCavePortals).
 */
class FUghLevelArt
{
public:
	/** A tile is this big (pixels). */
	static constexpr int32 TileWidth = 16, TileHeight = 12;
	/** The sprites of the boards: I, II, III, IIII, the four crossed by a fifth (FirstSign + marks - 1); a blank one. */
	static constexpr int32 FirstSign = 85, LastSign = 90;

	/** Takes the tiles of the levels' file (UghJson::LevelsFile) read from `Path`; false and the reason when not. */
	bool Load(const FJsonObject& LevelsFile, const FString& Path, FString& OutError);

	/** The drawing of level `LevelId`: the screen's pixels row by row (black when the level is unknown). */
	TArray<FColor> Draw(int32 LevelId, const FUghSprites& Sprites) const;

	/** The tiles of level `LevelId` that are boards with a pad's number (or a blank one). */
	TArray<FUghArtTile> Signs(int32 LevelId) const;
	/**
	 * The doors of level `LevelId` (where its passengers come out and go in): the top left tile of each, 2 x 2 tiles
	 * of a doorway of posts and a lintel or of a cave's mouth.
	 */
	TArray<FUghArtTile> Doors(int32 LevelId) const;

private:
	/** The top left tiles of the doors: a doorway (60, 61 over 80, 81), a cave's mouth (62, 63 over 82, 83). */
	static constexpr int32 Doorway = 60, CaveMouth = 62;

	/** The tiles of level `LevelId` whose sprite `Takes`. */
	TArray<FUghArtTile> TilesWhere(int32 LevelId, TFunctionRef<bool(int32 Sprite)> Takes) const;

	TArray<TArray<int32>> Tiles;   // by level_id: the tiles row by row
	int32 Columns = 0;
};
