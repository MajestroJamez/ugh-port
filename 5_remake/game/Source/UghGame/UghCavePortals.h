// The cave entrances of a level: the doors of the original's drawing, where the passengers come out and go in.
#pragma once

#include "CoreMinimal.h"
#include "UghLevelArt.h"

class FUghRockOutline;

/**
 * A cave entrance of the rock (FUghRockField) where the original's drawing has a door: an arch of fractured rock
 * standing out of the cave's back wall around a dark opening, whose passage goes deep into the cliff and turns aside
 * (no light gets to its end), its floor the ledge's. Pixels of the screen (y down); depths in pixels behind the plane
 * of the play.
 */
struct FUghCavePortal
{
	/** Its opening: half as wide and as high above the floor (pixels); the rock of the arch around it this thick. */
	static constexpr double HalfWidth = 11, Height = 22, Arch = 8;
	/**
	 * The arch's front is this deep in its middle (pixels): behind the sweep of a copter's rotor and of the flyer's
	 * wings (UghDecorations::SweepReach), so nothing that flies past touches it. Its passage ends End deep. It changes
	 * the rock from Nearest back.
	 */
	static constexpr double Front = 18, End = 72, Nearest = Front - 4;

	/** The middle of its opening and the surface of the floor under it. */
	double X = 0, Floor = 0;
	/** Which way its passage turns deeper in: -1 to the left, 1 to the right. */
	double Turn = 1;

	/** Where its passage is on the screen, all of it (pixels): nothing else may be seen in it. */
	FBox2D Passage() const;
	/** Where it may change the rock on the screen (pixels). */
	FBox2D Reach() const;
	/** How dark its passage is at `Point` (x, y, depth) of its walls, 0 .. 1: no light gets far in. */
	double Darkness(const FVector& Point) const;
	/** The field of the rock (positive in the rock) at `Point` (x, y, depth) with the arch and its passage. */
	float Shape(const FVector& Point, float Field) const;

	bool operator==(const FUghCavePortal& Other) const = default;
};

namespace UghCavePortals
{
	/**
	 * The entrances of the level being played: one for each door of its drawing (`Doors`, FUghLevelArt::Doors), its
	 * floor where the collision mask (`Outline`) has one under the door.
	 */
	TArray<FUghCavePortal> Plan(const FUghRockOutline& Outline, TConstArrayView<FUghArtTile> Doors);
}
