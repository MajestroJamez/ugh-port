// Where the decorations of a level stand.
#pragma once

#include "CoreMinimal.h"

struct ugh_logic;

/** A decoration of a level's diorama: a palm or a rock on a ledge, behind the slab of the play. */
struct FUghDecoration
{
	enum class EKind : uint8 { Palm, Rock };

	EKind Kind = EKind::Rock;
	/** Pixels: the middle of its foot, on the surface of its ledge (row Y). */
	int32 X = 0, Y = 0;
	/** Pixels: the box it fits in, Width across (and as deep), Height up from Y. */
	int32 Width = 0, Height = 0;
	/** Units behind the plane of the play: its middle (its front is behind the slab of the play). */
	double Depth = 0;
	/** Degrees it is turned about the vertical. */
	double Yaw = 0;
	/** Which of the meshes of its kind (modulo their count). */
	int32 Variant = 0;

	bool operator==(const FUghDecoration& Other) const = default;
};

/**
 * The decorations of the level being played: a palm (where one fits) and a couple of rocks, each on a dry ledge
 * (UghLedges) with room above for its whole box, away from the pads (a palm may stand on a pad's ledge where it
 * fits nowhere else, PalmOverPadMin) and the campfire, behind the slab of the play so that it never hides a figure.
 * The same level always gets the same ones (its level_id seeds the choice).
 */
namespace UghDecorations
{
	/** The gap between the slab of the play and the front of a decoration, units. */
	constexpr double SlabGap = 5;
	/**
	 * Where no palm fits off the pads, one at least this tall (pixels) may stand on a pad's ledge: its crown is then
	 * above the pad's sign (the original's drawing, on the back wall), only the thin trunk passes in front of it.
	 */
	constexpr int32 PalmOverPadMin = 28;

	TArray<FUghDecoration> Plan(const ugh_logic* Logic, int32 LevelId, int32 WaterRow, const TOptional<FIntPoint>& Hearth);
}
