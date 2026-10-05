// The rock of a level as ground to put things on.
#pragma once

#include "CoreMinimal.h"

class FUghRockField;
struct FUghDecoration;
struct ugh_logic;

/**
 * Where the rock of the level being played is, at any depth (its field, FUghRockField: behind the slab of the play
 * the floors are where the collision mask has them, give or take its roughness, the walls and ceilings reach into
 * the cave): the surface of a floor or a ceiling near a row of the mask, and whether a box is all air.
 */
class FUghGround
{
public:
	FUghGround(const ugh_logic* InLogic, const FUghRockField& InField) : Logic(InLogic), Field(InField) {}

	const ugh_logic* GetLogic() const { return Logic; }
	/** Pixel x, y of the collision mask is rock. */
	bool Solid(int32 X, int32 Y) const;

	/**
	 * The y of the floor's surface at pixel column `X` and `Depth` units behind the plane, within Reach pixels of row
	 * `Y` (the surface of a ledge of the mask); unset when there is none (rock or air all the way).
	 */
	TOptional<double> Floor(double X, double Y, double Depth) const;
	/** The y of a ceiling's surface above x, y (in the air) at `Depth` units, within CeilingReach; unset when none. */
	TOptional<double> Ceiling(double X, double Y, double Depth) const;
	/** How deep (units) the rock behind x, y begins, looking back from `Depth` (in the air); unset when it does not. */
	TOptional<double> Wall(double X, double Y, double Depth) const;
	/**
	 * The front half of the box of `Decoration` is air (sampled through its inner part): only where it stands (or
	 * hangs from) it may touch the rock; its back may lean on the cave's back wall.
	 */
	bool Clear(const FUghDecoration& Decoration) const;
	/** `Box` on the screen (pixels) is seen in the passage of a cave's entrance (FUghCavePortal). */
	bool AtEntrance(const FBox2D& Box) const;

	/** How far from a mask's row a floor is looked for, a ceiling, pixels. */
	static constexpr double Reach = 3, CeilingReach = 6;

private:
	/** The field at x, y (pixels) and depth (units) is rock. */
	bool Rock(double X, double Y, double Depth) const;

	const ugh_logic* Logic;
	const FUghRockField& Field;
};
