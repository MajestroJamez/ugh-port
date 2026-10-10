// The stone a copter carries in its sling, over the ground, and lets go, seen falling from the sling.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghCopterModel.h"
#include "UghShapes.h"

/** Where the stone in the sling is seen, in the frame of the copter's body (cm, UghCopterModel). */
struct FUghSlingPose
{
	FVector Stone = UghCopterModel::Hanging;   // its origin (its bottom middle)
	FQuat Turn = FQuat::Identity;              // its turn and the sling's (the sway)
	FVector Sling = FVector::ZeroVector;       // the sling's origin (the stone at Hanging in its frame)
	FVector Knot = FVector::ZeroVector;        // where the rope from the hook (SlingHook) ends
	double Lift = 0;                           // how far the ground lifted it from where it would hang
};

/**
 * The stone (the standing passenger) in a copter's sling over the ground. The logic hides it while it hangs and knows
 * nothing of it below the copter: it lands the copter with it as without (on its skids, CopterShape::SKIDS), and
 * takes it on the sling with the copter's skids just above the ground - seen hanging it would be up to its eyes in
 * the rock. Seen, it never goes into the ground (the collision mask under its columns): it hangs freely as long as
 * there is room, else it rests on the ground and, the lower the copter, is pushed aside along the ground (on the side
 * where the mask leaves it room and ground, towards the camera where neither does) as far as it must so as not to go
 * into the body; the rope from the floor's hook to the sling stretched to it. Only decoration.
 */
namespace UghSling
{
	/** The stone's columns either side of its middle, its rows (pixels); the mask is searched this far down. */
	constexpr double HalfColumns = 8, Rows = 11;
	constexpr int32 Look = 16;
	/** Beside the copter the ground may be this many pixels lower than under it. */
	constexpr int32 Step = 3;
	/** Aside it clears the body (its half width, the stone's, a margin); towards the camera its depth (cm). */
	constexpr double SideClear = (UghShapes::CopterBodyRight - UghShapes::CopterBodyLeft + 1) *
		UghShapes::UnitsPerPixel / 2 + UghCopterModel::StoneHalfWidth + 6;
	constexpr double FrontClear = UghCopterModel::BodyHalfDepth + UghCopterModel::StoneHalfDepth + 8;
	/** It is pushed aside when its top comes up from this far below the body's bottom to this far above (cm). */
	constexpr double PushFrom = -10, PushTo = 30;

	/**
	 * How far (pixels, -1 .. Look) the ground is below `Bottom` (the copter body's bottom row, fractions allowed) under
	 * the stone with its middle at `Middle`: the highest first solid pixel of the collision mask of `Logic` in its
	 * columns (below 0: the logic lets a copter stand a fraction of a pixel in the ground).
	 */
	double Room(const ugh_logic* Logic, double Middle, double Bottom);
	/**
	 * The side (-1 left, 1 right; `Prefer` first) the stone under a copter (its middle `Middle`, its body's bottom
	 * `Bottom`) can be pushed to along the ground under it: no rock in its way there and ground under it at most Step
	 * pixels lower; 0 on neither.
	 */
	int32 Side(const ugh_logic* Logic, double Middle, double Bottom, int32 Prefer);
	/**
	 * The stone swayed `Sway` radians about the copter's origin, pushed `Push` cm (aside along X, towards the camera
	 * along Y), with `Room` pixels of air under it.
	 */
	FUghSlingPose Pose(double Sway, double Room, const FVector2D& Push);
	/** How far (0 .. 1) it wants to be pushed when it is as `Pose` has it. */
	double Pushed(const FUghSlingPose& Pose);
	/**
	 * The stone in the sling of a copter in the level of `Logic` (its middle `Middle`, its body's bottom `Bottom`,
	 * pixels), swayed `Sway`: `Push` (cm, kept between frames) goes towards where it wants to be, keeping `Keep` (0 ..
	 * 1) of where it was; it is never in the ground under it.
	 */
	FUghSlingPose Follow(const ugh_logic* Logic, double Middle, double Bottom, double Sway, FVector2D& Push,
		double Keep);
}

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
	 * the logic may have taken a few steps by then, a frame is drawn after several); never more than `Room` pixels (the
	 * ground under where the logic has it: let go from a copter on the ground, it falls from the ground, not into it).
	 */
	double Below(int32 Index, double Top, bool bFalling, double Room = UE_DOUBLE_BIG_NUMBER);
	/** After the stones of a frame: those not seen are forgotten. */
	void End();

private:
	struct FDrop
	{
		double Start = 0, Fallen = 0;   // pixels: its top when it was let go, how far it has fallen at most
		double From = 0;                // how far below the logic it was seen then (SlingBelow, less on the ground)
		bool bSeen = false;
	};
	TMap<int32, FDrop> Drops;
};
