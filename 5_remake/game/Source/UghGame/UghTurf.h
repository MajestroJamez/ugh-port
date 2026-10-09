// The turf on the tops of the rock: grass hanging over their edges, the paths trodden to the cave entrances.
#pragma once

#include "CoreMinimal.h"
#include "UghRockMesh.h"

class FUghRockField;
struct FUghCavePortal;
struct FUghStream;
struct ugh_logic;

/**
 * The turf of the level being played (step 30b), made with its rock (FUghRockField): the paths trodden to the cave
 * entrances and the grass hanging over the top edges of the rock's face.
 *
 * - Paths: a mask over the screen (0 .. 255 a pixel), along the ledge under each entrance - full in front of its
 *   opening, fading out along the ledge either way (its length here shorter, there longer) -, from a few pixels above
 *   the edge (the ledge's top and the cave's floor behind it) to a few below it (the edge itself worn). The cliff's
 *   material (UghCliff.hlsl) shows bare packed soil there instead of grass (AUghBackground: the alpha of the drawing it
 *   gets, 255 - the mask).
 * - Blades: cards of grass blades (M_UghTurf draws the blades, UghTurf.hlsl) rooted on the top of the face just in front
 *   of the slab of the play, reaching forward over its edge and hanging down the face, each its own length (Shortest ..
 *   Longest; shorter on a path, none where it is trodden bare), never above the ledge's surface (they cannot hide a
 *   figure's feet or the surface a copter lands on) and only ever in front of the rock of the mask (never over the
 *   air where the figures fly): a card hangs only as far down as the rock under its whole width reaches. None under
 *   the water at the level's start, none where a stream pours over the edge. Vertex colours: red the card's seed,
 *   green how far along it (0 at the root), blue how trodden. One static mesh, no shadows: cheap at any quality.
 */
class FUghTurf
{
public:
	/** Pixels: a card's width, how far apart they are along an edge; how long they hang, at most, at least. */
	static constexpr double CardWidth = 1.5, CardStep = 1.1, Longest = 6.5, Shortest = 1.2;
	/**
	 * Pixels: the root RootBehind behind the face's lip, never deeper than RootDepth (just in front of the slab of the
	 * play); how far in front of the face the blades hang, at the lip and at the tip.
	 */
	static constexpr double RootDepth = -2.4, RootBehind = 0.8, Off = 0.25, TipOff = 1.8;
	/** Pixels: a path is full this far from the middle of an entrance, fading out this much further (at most). */
	static constexpr double PathCore = 8, PathFade = 22;
	/** Rows of a path above and below the edge. */
	static constexpr int32 PathAbove = 5, PathBelow = 3;

	/** The turf of the rock `Field` of the level of `Logic` (its water at the start at `WaterRow`, its `Streams`). */
	void Build(const ugh_logic* Logic, const FUghRockField& Field, int32 WaterRow, TConstArrayView<FUghStream> Streams);

	/** The paths' mask, ScreenWidth x ScreenHeight. */
	const TArray<uint8>& GetPaths() const { return Paths; }
	uint8 PathAt(int32 X, int32 Y) const;
	/** The blades' cards in the world (FUghRockMesh only carries them: vertices, normals, UVs, colours, triangles). */
	const FUghRockMesh& GetBlades() const { return Blades; }
	int32 CardCount() const { return Cards; }

	/** The paths to the entrances `Portals` of the level of `Logic`. */
	static TArray<uint8> MakePaths(const ugh_logic* Logic, TConstArrayView<FUghCavePortal> Portals);
	/** Pixel x, y is the top of the rock: solid in the collision mask, air above it. */
	static bool Edge(const ugh_logic* Logic, int32 X, int32 Y);

private:
	/** A card at x on the edge of row `Y` hanging `Length` pixels (the face of `Field`). */
	void AddCard(const FUghRockField& Field, double X, int32 Y, double Length, double Seed, double Trodden);

	TArray<uint8> Paths;
	FUghRockMesh Blades;
	int32 Cards = 0;
};
