// The boards with the pads' numbers.
#pragma once

#include "CoreMinimal.h"
#include "UghDecorations.h"
#include "UghLevelArt.h"

class FUghGround;
struct ugh_logic;

/** A pad's board: whose it is, what is carved in it and where it stands. */
struct FUghPadSign
{
	/** The pad's index (ugh_logic_get_pad) and number. */
	int32 Pad = 0, Number = 0;
	/** The tally marks carved in it (UghPadSigns::Marks): its pad's number, none on a blank board. */
	int32 Marks = 0;
	/** The board the original draws on the pad (a sprite of the drawing's tiles); unset when it draws none. */
	TOptional<int32> Sprite;
	/** Pixels: the middle of its post and the ground it stands on there. */
	double X = 0, Y = 0;

	/** Pixels: what it covers on the screen, as the original's tile (16 x 12 px, its post 7.5 px from its left). */
	FBox2D Box() const;

	bool operator==(const FUghPadSign& Other) const = default;
};

/**
 * Every pad of the level being played gets exactly one board with its number, standing on the pad behind the slab of
 * the play and out of the figures' reach (Front .. Back, units), so that no copter's body or enemy goes through it:
 * where the original's drawing has a board on the pad (its bottom on the pad's surface, the tile over the pad; the
 * first of two), else as near the middle of the pad as it is out of the caves' entrances and of the other boards. The
 * marks are the pad's number as the original's sprites draw it: one to four strokes, the four crossed by a fifth; a
 * pad of number 6 or more (the original's pads that no passenger asks for by number) gets its blank board.
 */
namespace UghPadSigns
{
	/** Pixels: a board covers the original's tile, its post's middle this far from the tile's left. */
	constexpr double Width = FUghLevelArt::TileWidth, Height = FUghLevelArt::TileHeight, PostX = 7.5;
	/**
	 * Units behind the plane of the play: the foot of its post (the model's origin); the model (Blender/signs.py)
	 * reaches 6 units in front of it (the pegs) and 18 behind it (the post), a little more is kept free.
	 */
	constexpr double Depth = UghDecorations::FigureReach + 8, Front = Depth - 7, Back = Depth + 20;
	static_assert(Front >= UghDecorations::FigureReach, "a board stands out of the figures' reach");
	/** The most tally marks a board has (the original's last numbered board). */
	constexpr int32 MostMarks = 5;

	/** The marks for pad number `Number`: itself, none above MostMarks. */
	int32 Marks(int32 Number);
	/** The marks of the original's board `Sprite` (FUghLevelArt::FirstSign ..), unset when it is no board. */
	TOptional<int32> MarksOfSprite(int32 Sprite);
	/** The sprite of the original's board with `Marks` (its blank board for none). */
	int32 SpriteOf(int32 Marks);

	/**
	 * The boards of the level being played (its rock `Ground`, its drawing's boards `Drawn`, FUghLevelArt::Signs), in
	 * the order of its pads; none before a level.
	 */
	TArray<FUghPadSign> Plan(const ugh_logic* Logic, const FUghGround& Ground, const TArray<FUghArtTile>& Drawn);
}
