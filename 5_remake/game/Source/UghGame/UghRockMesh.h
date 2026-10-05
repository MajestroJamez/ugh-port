// The rock of a level as a mesh.
#pragma once

#include "CoreMinimal.h"
#include "UghRockField.h"
#include "UghShapes.h"
#include "UghSurfaceNets.h"

class UStaticMesh;

/**
 * The rock of the level being played, a smooth cliff with its cave: the surface of FUghRockField (UghSurfaceNets) in
 * the world. In the plane of the play its edge is the collision mask's (test Ugh.Rock); in front of the slab of the
 * play the rock's face, behind it the cave reaching to its back wall, beyond the screen the cliff going on. The
 * normals follow the field (smooth), the UVs map the screen onto the drawing of the level (FUghLevelArt), the vertex
 * colours tell the material how open the surface is (red: 1 open, 0 in a crevice or deep in the passage of a cave's
 * entrance), how deep (green: 0 at the slab, 1 at the back wall), how near below the rock's top edge in the plane of
 * the play (blue: 1 at the edge, 0 a few pixels below it and behind the slab: grass and moss hang over it) and its
 * large patches (alpha: 0 .. 1 over metres).
 */
class FUghRockMesh
{
public:
	/** The front of the rock and its deepest back wall, units (depths of UghShapes). */
	static constexpr double FrontDepth = FUghRockField::FrontDepth * UghShapes::UnitsPerPixel;
	static constexpr double BackDepth = FUghRockField::BackDepth * UghShapes::UnitsPerPixel;
	/** How far the rock reaches beyond the screen's edges, pixels. */
	static constexpr int32 MarginX = FUghRockOutline::MarginX, MarginY = FUghRockOutline::MarginY;

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Triangles;

	/** The surface of `Field` (none when it is empty). */
	void Build(const FUghRockField& Field);
	/**
	 * The surface of the points and quads of UghSurfaceNets (pixels) in the world: each point's normal the way
	 * `Outward` says the surface faces there (pixels), its colour `Color`'s (the point, that way).
	 */
	void Build(const TArray<FVector>& Points, const TArray<FUghNetQuad>& Quads,
		TFunctionRef<FVector(const FVector&)> Outward, TFunctionRef<FColor(const FVector&, const FVector&)> Color);
	/**
	 * The mesh as the engine's static mesh (one material slot): drawn, shadowed and ray traced like any static mesh,
	 * so its shadows and ray tracing are built once, not every frame. None when empty.
	 */
	UStaticMesh* ToStaticMesh(UObject* Outer) const;
	/** The large patches at a point (pixels), 0 .. 1 over metres (the vertex colours' alpha). */
	static double Patches(const FVector& Point);

private:
	/** The colour of a vertex at `Point` (pixels) whose surface faces `Outward`. */
	static FColor Shade(const FUghRockField& Field, const FVector& Point, const FVector& Outward);
};
