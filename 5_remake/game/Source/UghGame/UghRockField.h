// The rock of a level as a field in 3D.
#pragma once

#include "CoreMinimal.h"
#include "UghRockOutline.h"
#include "UghShapes.h"

struct FUghRockStamp;
struct ugh_logic;

/**
 * The rock of the level being played as a field on a grid: positive in the rock, negative in the air, its surface
 * where it is zero (FUghRockMesh). The grid's columns and rows are the centres of the pixels (FUghRockOutline, beyond
 * the screen too), its layers the Depths (pixels; negative towards the camera, 0 the plane of the play).
 *
 * - In the slab of the play (|depth| <= SlabHalf) the field is the outline's distance: the rock is the collision mask
 *   exactly, its edge on the pixels' borders.
 * - In front of the slab the rock's face, rounded at its edges and bulging a little in its middle.
 * - Behind it the rock follows the blurred outline, its walls and ceilings reaching further into the cave the deeper
 *   they are (overhangs) and rough, the cave's back wall far behind (deeper where the drawing has a dark hole), the
 *   stalactites and fallen rocks of UghRockFeatures.
 * - Beyond the screen's edges the rock closes in: the cliff goes on.
 */
class FUghRockField
{
public:
	/** The slab of the play reaches this far in front of and behind its plane, pixels. */
	static constexpr double SlabHalf = UghShapes::PlaneThickness / 2 / UghShapes::UnitsPerPixel;
	/** The rock's face is at most this far in front of the slab, pixels. */
	static constexpr double FaceMax = 4;
	/** The front of the rock, its deepest back wall, pixels. */
	static constexpr double FrontDepth = -SlabHalf - FaceMax, BackDepth = 78;

	/** The depths of the grid's layers, front to back, pixels. */
	static TConstArrayView<double> Depths();

	/**
	 * The rock of the level being played with its stalactites and fallen rocks (UghRockFeatures); `Art` the drawing of
	 * the level (screen-sized, else no holes in the back). None before a level.
	 */
	void Build(const ugh_logic* Logic, TConstArrayView<FColor> Art);
	/** No level was built. */
	bool IsEmpty() const { return Values.IsEmpty(); }

	static constexpr int32 Columns = FUghRockOutline::Width, Rows = FUghRockOutline::Height;
	static int32 Layers() { return Depths().Num(); }
	float At(int32 I, int32 J, int32 K) const { return Values[(K * Rows + J) * Columns + I]; }
	/** The field at x, y (pixels of the screen) and depth (pixels), between the grid's points; clamped to the grid. */
	float Sample(const FVector& Point) const;
	/** Which way the field grows at the point (per pixel). */
	FVector Gradient(const FVector& Point) const;

private:
	/** The field in front of the slab, in it and behind it, at column I, row J. */
	float Front(int32 I, int32 J, double Depth) const;
	float Behind(int32 I, int32 J, double Depth) const;
	/** How much the rock has closed in at column I, row J (0 on the screen). */
	static double Closing(int32 I, int32 J);
	void MakeBackWall(TConstArrayView<FColor> Art);
	void Stamp(const FUghRockStamp& Stamp);

	FUghRockOutline Outline;
	TArray<float> BackWall;   // its depth at each column and row
	TArray<float> Values;
};
