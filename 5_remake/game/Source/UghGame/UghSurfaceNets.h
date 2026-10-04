// The surface of a field as a mesh of quads.
#pragma once

#include "CoreMinimal.h"

class FUghRockField;

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
 * so in the slab of the play every pixel's centre stays on its side of the edge.
 */
namespace UghSurfaceNets
{
	/** The points (x, y, depth in pixels, as FUghRockField) and the quads. */
	void Build(const FUghRockField& Field, TArray<FVector>& OutPoints, TArray<FUghNetQuad>& OutQuads);
}
