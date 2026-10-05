// The stone the level is carved into, as a field.
#pragma once

#include "CoreMinimal.h"
#include "UghDecorations.h"
#include "UghRockField.h"

/**
 * The sea stack the level is carved into (AUghSeaStack): a tower of grey karst limestone standing in the open sea, as
 * a field (positive in the stone; pixels of the screen x, y and depth, as FUghRockField) on a grid Cell pixels apart.
 * Its face is flat around the screen: a frame a little in front of the level's rock (FUghRockField) around the hollow
 * the rock sits in (Hole: within the rock's grid, so that the rock's open border is inside the stone), the face
 * standing out further the further it is from the hollow; its sides and back rounded, fluted by the rain, fractured in
 * blocks, lumpy; its top a dome where the jungle grows (Plants); its foot spreading under the sea. It holds the unseen
 * shroud of AUghBackground too. The same for every level.
 */
class FUghStackField
{
public:
	/** The grid: its first node, its nodes this many pixels apart, how many along x, y and the depth. */
	static constexpr double Cell = 6;
	static constexpr int32 Columns = 236, Rows = 199, LayerCount = 134;
	static inline const FVector Origin = FVector(-545, -850, -120);
	/** The hollow of the level's rock: pixels of the screen, from far in front back to HoleBack (pixels). */
	static constexpr double HoleLeft = -43, HoleRight = UghShapes::ScreenWidth + 43, HoleTop = -19,
		HoleBottom = UghShapes::ScreenHeight + 19, HoleBack = FUghRockField::BackDepth + 8;
	/** The frame around it is at least this far in front of the plane of the play (pixels: the rock's face at most 6). */
	static constexpr double FrameDepth = -10;

	static int32 Layers() { return LayerCount; }
	static FVector Node(int32 I, int32 J, int32 K) { return Origin + FVector(I, J, K) * Cell; }

	/** The field at the grid's nodes. */
	void Build();
	bool IsEmpty() const { return Values.IsEmpty(); }
	float At(int32 I, int32 J, int32 K) const { return Values[(K * Rows + J) * Columns + I]; }
	/** The field between the grid's nodes (what its surface is), clamped to the grid. */
	float Sample(const FVector& Point) const;

	/** The stone's field at a point (pixels): positive in the stone, about how far from its surface. */
	static double Value(const FVector& Point);
	/** Which way the field grows at the point (into the stone). */
	static FVector Gradient(const FVector& Point);
	/**
	 * The vertex colour of its surface at `Point` facing `Outward` (pixels), as FUghRockMesh's: how open (red), at the
	 * face (green 0), how near below its top (blue: the grass hangs over it), its large patches (alpha).
	 */
	static FColor Shade(const FVector& Point, const FVector& Outward);
	/**
	 * The jungle of the stone (shown by AUghScenery): palms, bushes, jungle plants, ferns and rocks on its top, bushes,
	 * plants and ferns on the ledges of its walls, creepers hanging down their tops (at some of `Points`, its surface)
	 * away from the level; the same every time.
	 */
	TArray<FUghDecoration> Plants(const TArray<FVector>& Points) const;

private:
	/** The plain shape at a point: how far from the hollow, where its face and its top are, how far outside it. */
	struct FShape
	{
		double Hole, Front, Top, Outside;
	};
	static FShape Shape(const FVector& Point);

	TArray<float> Values;
};
