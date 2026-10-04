// Where things fit on the rock of a level.
#pragma once

#include "CoreMinimal.h"

struct ugh_logic;

/** A dry stretch of the top of the rock: pixels First .. Last - 1 of row Y (solid, air above). */
struct FUghLedge
{
	int32 Y = 0, First = 0, Last = 0;

	int32 Length() const { return Last - First; }
};

/**
 * The tops of the rock of the level being played (its collision mask) where decoration fits: dry, with room above,
 * mostly with no pad on them. Only the frontend's: the game does not know them.
 */
namespace UghLedges
{
	/** A pad margin of Find that lets the pads' ledges count too. */
	constexpr int32 PadsToo = -1;

	/**
	 * The ledges above `WaterRow` with `Room` empty pixels above every pixel and no pad on them or within `PadMargin`
	 * pixels of a pad's ends (PadsToo: pads or not).
	 */
	TArray<FUghLedge> Find(const ugh_logic* Logic, int32 WaterRow, int32 Room, int32 PadMargin);

	/** A pad of the level lies on row Y (or next to it) within `Margin` pixels of pixels Left .. Right - 1. */
	bool NearPad(const ugh_logic* Logic, int32 Left, int32 Right, int32 Y, int32 Margin);

	/** How many empty pixels are above pixel x, y (up to the top of the screen), at most `Limit`. */
	int32 RoomAbove(const ugh_logic* Logic, int32 X, int32 Y, int32 Limit);

	/**
	 * Where a campfire fits: the middle of the longest ledge above `WaterRow` (pixels: x, and y of its surface);
	 * unset when there is none.
	 */
	TOptional<FIntPoint> FindHearth(const ugh_logic* Logic, int32 WaterRow);
}
