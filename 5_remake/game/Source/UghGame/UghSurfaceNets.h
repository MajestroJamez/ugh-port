// The surface of a field as a mesh of quads.
#pragma once

#include "CoreMinimal.h"

/** A quad of the surface: its corners (indices of the points) in order around it, the way its outside faces. */
struct FUghNetQuad
{
	int32 Corners[4];
	/** A unit vector along an axis of the grid (pixels: x, y, depth), from the rock to the air. */
	FVector Outward;
};

/**
 * Surface nets: the surface where the field is zero. Every cell of the grid it passes gets a point (the mean of where
 * it crosses the cell's edges), every edge of the grid it crosses a quad of the points of the four cells around the
 * edge. A smooth mesh with no gaps; the points of the grid keep their side (every crossed edge is crossed by its quad),
 * so in the slab of the play every pixel's centre stays on its side of the edge. The surface does not close at the
 * grid's border.
 */
namespace UghSurfaceNets
{
	/**
	 * The points (x, y, depth in pixels, as FUghRockField) and the quads of the field of `Grid`: Columns x Rows x
	 * Layers() nodes, At(I, J, K) the field at a node (positive in the rock), Node(I, J, K) where it is. For
	 * FUghRockField (the level's rock) and FUghStackField (the stone around it).
	 */
	template <class TGrid>
	void Build(const TGrid& Grid, TArray<FVector>& OutPoints, TArray<FUghNetQuad>& OutQuads);
}
