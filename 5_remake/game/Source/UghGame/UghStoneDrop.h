// The stone a copter lets go, seen falling from its sling.
#pragma once

#include "CoreMinimal.h"
#include "UghCopterModel.h"
#include "UghShapes.h"

/**
 * The stone (the standing passenger) a copter lets go. In the logic it hangs hidden under the copter and, let go,
 * starts to fall from the copter's drop point (CopterShape::DROP, its middle DropMiddle pixels below the copter's top:
 * in the body, beside the pilot). Seen, it hung in the sling SlingBelow pixels lower: so it starts there and only
 * comes up to where the logic has it as it falls - seen lower by Below, which shrinks with how far it has fallen and is
 * gone once it fell CatchUp times that far (seen it falls slower at first, never up, out of the sling straight down;
 * the logic's stone hits an enemy below a copter from about 20 px of fall on, then seen at most a few pixels lower).
 * Only decoration: the logic's stone is where the logic has it.
 */
class FUghStoneDrops
{
public:
	/** The logic lets the stone go its middle this far below the copter's top (pixels; CopterShape::DROP). */
	static constexpr double DropMiddle = 10;
	/** Its bottom in the sling is this far below its bottom where the logic lets it go (pixels; 11 px high). */
	static double SlingBelow()
	{
		return UghShapes::CopterBodyHeight - UghCopterModel::Hanging.Z / UghShapes::UnitsPerPixel - (DropMiddle + 5.5);
	}
	/** It is where the logic has it once it fell this many times SlingBelow. */
	static constexpr double CatchUp = 1.5;

	/** Before the stones of a frame. */
	void Begin();
	/**
	 * How far below where the logic has it (pixels) stone `Index` is seen, its top at `Top` (pixels) now, `bFalling`
	 * while it falls or bounces: a fall begins where it is first seen falling (a stone falls only let go by a copter;
	 * the logic may have taken a few steps by then, a frame is drawn after several).
	 */
	double Below(int32 Index, double Top, bool bFalling);
	/** After the stones of a frame: those not seen are forgotten. */
	void End();

private:
	struct FDrop
	{
		double Start = 0, Fallen = 0;   // pixels: its top when it was let go, how far it has fallen at most
		bool bSeen = false;
	};
	TMap<int32, FDrop> Drops;
};
