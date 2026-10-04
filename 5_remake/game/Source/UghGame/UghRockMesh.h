// The rock of a level as a mesh.
#pragma once

#include "CoreMinimal.h"
#include "UghShapes.h"

struct ugh_logic;

/**
 * The rock of the level being played, cut in the plane of the play: the solid pixels of its collision mask are the
 * cut face (the front of the slab the figures move in) and reach back to the cave's back wall; the edges between
 * solid and empty pixels are the cave's walls, floors and ceilings, one pixel a step. Behind the whole screen is the
 * bumpy back wall. The UVs map the screen onto the drawing of the level (FUghLevelArt), so every face has the colour
 * of the original's pixel it belongs to.
 */
class FUghRockMesh
{
public:
	/** The cut face: the front of the slab of the play (UghShapes::PlaneThickness). */
	static constexpr double CutDepth = -UghShapes::PlaneThickness / 2;
	/** How deep the rock reaches behind the plane of the play, and how far the back wall's bumps come forward. */
	static constexpr double BackDepth = 300, BumpDepth = 80;

	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Triangles;

	/** Builds the rock of the level being played; nothing before the first one is loaded. */
	void Build(const ugh_logic* Logic);

private:
	/** The size of a cell of the back wall's grid, pixels. */
	static constexpr int32 WallStep = 4;

	void AddCut(const ugh_logic* Logic);
	void AddFloorsAndCeilings(const ugh_logic* Logic);
	void AddWalls(const ugh_logic* Logic);
	void AddBackWall();
	/** A quad of corners A B C D (in order around it) facing `Normal`, with the UVs of the screen pixels given. */
	void AddQuad(const FVector (&Corners)[4], const FVector2D (&Pixels)[4], const FVector& Normal);
	/** The back wall's depth at pixel x, y. */
	static double BackWallDepth(double X, double Y);
};
