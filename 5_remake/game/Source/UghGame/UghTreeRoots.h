// The long roots of the jungle tree with a face, growing down the rock's face.
#pragma once

#include "CoreMinimal.h"
#include "UghRockMesh.h"

class FUghRockField;
struct FUghStream;
struct ugh_logic;

/**
 * Where the trees of every level stand: the game data's tree records (assets/logic/ugh-data.ugd: `tree x=<x> y=<y>`
 * under `level <level_id>`, the top left corner of its sprite in the logic's subpixels), read once; the foot of each
 * tree is the middle of the bottom of its sprite (TreeSprite px), where its model stands (UghFigurePlace).
 */
class FUghTreePlaces
{
public:
	/** The tree's sprite (pixels; tree.swaying of the data). */
	static constexpr int32 TreeWidth = 32, TreeHeight = 24;

	/** Reads the data file `File`; false and the reason when it cannot. */
	bool Load(const FString& File, FString& OutError);
	/** The feet of the trees of level `LevelId` (pixels), none when it has none. */
	TArray<FVector2D> Of(int32 LevelId) const;

private:
	TMap<int32, TArray<FVector2D>> Feet;
};

/**
 * The roots of the trees of the level being played (step 32g), made with its rock (FUghRockField): from under each
 * tree's buttresses over the lip of its ledge and down the face of the rock, wandering, branching, thinning, the tip
 * into the rock - as a strangler fig's roots grow down a cliff. Only ever in front of the rock of the collision mask
 * (Margin px of rock around a root on either side, never over the air where the figures fly) and below the ledge's
 * surface (never over a figure's feet); none under the water at the level's start, none where a stream pours down.
 * Tubes of the tree's tiling bark (the material slot `roots` of the tree's model) lying on the face, a little sunk into
 * it. One static mesh, a few thousand triangles: cheap at any quality. The same level always gets the same roots.
 */
class FUghTreeRoots
{
public:
	/** A root: its middle line (pixels x, y and depth: negative towards the camera) and its radius there (pixels). */
	struct FStrand
	{
		TArray<FVector> Points;
		TArray<double> Radii;
	};

	/** Pixels: rock beside a root at least; a root's radius at its start, at most and at least; at its tip. */
	static constexpr double Margin = 1, Thickest = 1.25, Thinnest = 0.7, Tip = 0.2;
	/** Pixels: how far either way of a tree's foot its roots start; how long they grow at most, at least. */
	static constexpr double Spread = 11, Longest = 70, Shortest = 18;
	/** Pixels a root creeps along the face of a ledge it cannot go down from, at most. */
	static constexpr int32 Creep = 7;
	/**
	 * A root lies this much of its thickness sunk into the face as the field has it, and Out pixels further out (the
	 * rock's mesh comes out of the field a little: its relief).
	 */
	static constexpr double Sunk = 0.15, Out = 1.5;
	/** Sides of a root's tube; units of root a picture of the bark covers along it. */
	static constexpr int32 Sides = 6;
	static constexpr double BarkTile = 60;

	/**
	 * The roots of the trees standing at `Feet` (pixels) in the level of `Logic` (its rock `Field`, its water at the
	 * start at `WaterRow`, its `Streams`), `LevelId` seeding them.
	 */
	void Build(const ugh_logic* Logic, const FUghRockField& Field, TConstArrayView<FVector2D> Feet, int32 WaterRow,
		TConstArrayView<FUghStream> Streams, int32 LevelId);

	const TArray<FStrand>& GetStrands() const { return Strands; }
	/** The tubes in the world (FUghRockMesh only carries them). */
	const FUghRockMesh& GetMesh() const { return Mesh; }

private:
	void AddTube(const FStrand& Strand);

	TArray<FStrand> Strands;
	FUghRockMesh Mesh;
};
