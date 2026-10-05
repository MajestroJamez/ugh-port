// Where the decorations of a level stand.
#pragma once

#include "CoreMinimal.h"
#include "UghShapes.h"

class FUghRockField;
struct FUghPadSign;
struct ugh_logic;

/** A decoration of a level's diorama, on the rock behind the slab of the play. */
struct FUghDecoration
{
	enum class EKind : uint8
	{
		Grass, Flower, Rock, Bones, Fern, Bush, Plant, Stump, Palm, Totem, Hut,
		/** A liana hanging from a ceiling. */
		Vine,
		/** A liana hanging down the cave's back wall (from where it is: Y its top). */
		Creeper,
		/** A campfire (AUghCampfire shows them): its flame, a light. */
		Campfire
	};

	EKind Kind = EKind::Rock;
	/** Pixels: the middle of its foot on the ground (Y the ground's surface); a vine: where it hangs from. */
	double X = 0, Y = 0;
	/** Pixels: the box it fits in, Width across (and as deep), Height up from Y (a vine: down from Y). */
	double Width = 0, Height = 0;
	/** Units behind the plane of the play: the middle of its box. */
	double Depth = 0;
	/** Degrees it is turned about the vertical. */
	double Yaw = 0;
	/** Which of the meshes of its kind (modulo their count). */
	int32 Variant = 0;

	bool Hangs() const { return Kind == EKind::Vine || Kind == EKind::Creeper; }
	/** Its box, pixels of the screen (y down) and units behind the plane of the play. */
	double Left() const { return X - Width / 2; }
	double Right() const { return X + Width / 2; }
	double Top() const { return Hangs() ? Y : Y - Height; }
	double Bottom() const { return Hangs() ? Y + Height : Y; }
	double Front() const { return Depth - Width * UghShapes::UnitsPerPixel / 2; }
	double Back() const { return Depth + Width * UghShapes::UnitsPerPixel / 2; }

	bool operator==(const FUghDecoration& Other) const = default;
};

/**
 * The decorations of the level being played: campfires, palms, totems, huts, bushes, ferns, jungle plants, rocks,
 * stumps and bones on the dry tops of the rock, meadows of grass and flowers along them, lianas from its ceilings and
 * down the cave's back wall -
 * each standing on the rock (FUghRockField, at its own depth) with room for its whole box, never hiding a figure:
 *
 * - nothing comes nearer the plane of the play than SlabFront (the slab of the play, where the figures are, and the
 *   boards with the pads' numbers just behind it),
 * - nothing taller than GroundCover nearer than FigureReach (the copters' bodies and the enemies reach that deep),
 * - nothing taller than Middle, and no liana, nearer than SweepReach (a copter's rotor and the flyer's wings),
 * - where a copter lands on a pad nothing nearer than FigureReach where its body is (PadBody), than SweepReach where
 *   its rotor sweeps (PadRotor),
 * - nothing in front of a pad's board (UghPadSigns: nothing nearer than its Back where it stands).
 *
 * The same level always gets the same ones (its level_id seeds the choice).
 */
namespace UghDecorations
{
	constexpr double SlabFront = UghShapes::PlaneThickness / 2 + 5;
	/** The copters' bodies reach 45 units behind the plane, the triceratops, the blower and the tree with a face 70. */
	constexpr double FigureReach = 80;
	/** A copter's rotor sweeps 130 units around its hub, the flyer's wings reach 170. */
	constexpr double SweepReach = 180;
	/** Pixels: ground cover is at most this tall, middle decoration at most Middle. */
	constexpr double GroundCover = 5, Middle = 16;
	/**
	 * Where a copter landing on a pad is: pixels beside the pad's ends, from Top above its surface down to Bottom above
	 * it; how near the plane of the play nothing may come there (units). Its body (22 x 20 px) reaches half its width
	 * beyond a pad, its rotor (at 18.7 px) 13 px; ground cover stays out of its floor.
	 */
	struct FPadRoom
	{
		double Side, Top, Bottom, Front;
	};
	constexpr FPadRoom PadBody{ 11, 20, 0, FigureReach }, PadRotor{ 13, 24, 16, SweepReach };

	/** The name of a kind ("grass", "campfire"). */
	const TCHAR* Name(FUghDecoration::EKind Kind);
	/** How many there are of each kind ("grass 900, palm 2, ..."). */
	FString Summary(const TArray<FUghDecoration>& Decorations);

	/** How near the plane of the play the front of `Decoration` may come (units), wherever it is. */
	double NearestFront(const FUghDecoration& Decoration);

	/**
	 * The decorations of the level being played (its rock `Field`, level `LevelId`, the water's surface at the start at
	 * `WaterRow`, the pads' boards `Signs`); none before a level.
	 */
	TArray<FUghDecoration> Plan(const ugh_logic* Logic, const FUghRockField& Field, int32 LevelId, int32 WaterRow,
		const TArray<FUghPadSign>& Signs);
}
