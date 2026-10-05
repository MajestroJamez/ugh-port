// The rules every decoration of a level is placed by.
#pragma once

#include "CoreMinimal.h"
#include "UghDecorations.h"

class FUghGround;
struct FUghPadSign;
struct FUghStream;

/**
 * The decorations of a level placed so far, and whether another one fits (UghDecorations): on the screen above the
 * water, settled on the rock at its own depth (its foot on a floor, a liana under a ceiling, a torch wedged into the
 * wall behind it) with its box all air,
 * as far behind the plane of the play as it must be there (UghDecorations::NearestFront, and more where a copter lands
 * on a pad, behind the pads' boards where they stand, off the streams' beds and bridges), not in another one's box
 * (ground cover among ground cover may be), nowhere near a campfire, nowhere in front of a cave's entrance but ground
 * cover on the floor in front of its arch (it stays open: the passengers come out of it).
 */
class FUghPlacer
{
public:
	FUghPlacer(const FUghGround& InGround, int32 InWaterRow, const TArray<FUghPadSign>& Signs,
		const TArray<FUghStream>& InStreams);

	const FUghGround& GetGround() const { return Ground; }
	int32 GetWaterRow() const { return WaterRow; }
	const TArray<FUghStream>& GetStreams() const { return Streams; }
	const TArray<FUghDecoration>& GetPlaced() const { return Placed; }
	TArray<FUghDecoration> TakePlaced() { return MoveTemp(Placed); }

	/** How near the plane of the play the front of `Decoration` may come where it is (units). */
	double NearestFront(const FUghDecoration& Decoration) const;
	/** `Decoration` (Y the row of the mask it stands on or hangs from) settled on the rock and added if it fits. */
	bool TryAdd(FUghDecoration Decoration);
	/** TryAdd with `Decoration` so deep that its front is `Extra` units behind the nearest it may be there. */
	bool TryAddBehind(FUghDecoration Decoration, double Extra);

private:
	/**
	 * Where a copter landing on a pad may be, a pad's board stands or a stream keeps clear (pixels), and how near the
	 * plane of the play nothing may come there.
	 */
	struct FLanding
	{
		double Left, Right, Top, Bottom, Front;
	};

	/**
	 * Each pad's two: the copter's body and its rotor's sweep (UghDecorations::PadBody, PadRotor); each board; what
	 * each stream keeps clear (UghStreams::Rooms).
	 */
	static TArray<FLanding> Landings(const ugh_logic* Logic, const TArray<FUghPadSign>& Signs,
		const TArray<FUghStream>& Streams);
	/** Puts its foot on the floor at its depth (or its top under the ceiling); false when there is none, or uneven. */
	bool Settle(FUghDecoration& Decoration) const;
	/** It is seen in front of (or in) a cave's entrance, where it may not be. */
	bool AtEntrance(const FUghDecoration& Decoration) const;
	/** It is in the box of one placed (or near a campfire). */
	bool Crowds(const FUghDecoration& Decoration) const;

	const FUghGround& Ground;
	const int32 WaterRow;
	const TArray<FUghStream>& Streams;
	const TArray<FLanding> Pads;
	TArray<FUghDecoration> Placed;
};
