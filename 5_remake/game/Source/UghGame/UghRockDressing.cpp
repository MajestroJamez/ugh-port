#include "UghRockDressing.h"

#include "UghGround.h"
#include "UghRockField.h"

namespace
{
	using EKind = FUghRockPiece::EKind;

	/**
	 * Pieces of a kind keep their middles this much of their radii apart: the cliffs overlap into one wall of scanned
	 * rock, the roots into tangles.
	 */
	constexpr double Apart = 0.3;
	/**
	 * Cliffs: radii (pixels), one place tried every this many pixels (beyond the screen's edges too), how much comes
	 * out; turned at most this much (degrees) from facing the camera (their scans are open at the back).
	 */
	constexpr double CliffMin = 40, CliffMax = 65, CliffOutMin = 0.45, CliffOutMax = 0.7, CliffTurn = 15, CliffTilt = 6;
	constexpr int32 CliffStep = 24;
	/**
	 * Roots hanging from a ceiling: in about every RootEvery-th column of RootStep pixels under a ceiling with RootRoom
	 * pixels of air below, radii (pixels, at most RootReach of the room), how much comes out of the wall (or of a cliff
	 * in front of it); their models lie along their Y from the trunk's end: turned so that they hang (roll), then any
	 * way about the vertical and tilted a little.
	 */
	constexpr int32 RootStep = 5, RootEvery = 2, RootRoom = 14;
	constexpr double RootMin = 8, RootMax = 18, RootReach = 0.5, RootOutMin = 0.4, RootOutMax = 0.8, RootHang = -90;
	constexpr double RootTilt = 20;
	/** A piece seen in a cave's entrance gets smaller by ShrinkStep, down to Shrink of its radius (else none). */
	constexpr double Shrink = 0.4, ShrinkStep = 0.85;
	/** Units: a piece stays this far behind the middles of the decorations in front of it. */
	constexpr double DecorationGap = 10;
	/**
	 * A model in its ball reaches at most this much of the radius across the screen and up and down (the cliffs and
	 * the roots: their boxes' half widths and heights for the half diagonal, 0.54 .. 0.78).
	 */
	constexpr double ModelReach = 0.8;

	/** The nearest the cliffs of `Pieces` at x, y may come (units); unset where there is none. */
	TOptional<double> CliffFront(const TArray<FUghRockPiece>& Pieces, double X, double Y)
	{
		TOptional<double> Front;
		for (const FUghRockPiece& Cliff : Pieces)
		{
			const double Reach = Cliff.Radius * ModelReach;
			if (Cliff.Kind == EKind::Cliff && FMath::Abs(Cliff.X - X) < Reach && FMath::Abs(Cliff.Y - Y) < Reach)
			{
				Front = FMath::Min(Front.Get(UE_BIG_NUMBER), Cliff.Nearest());
			}
		}
		return Front;
	}

	/**
	 * Adds `Piece` (its X, Y given) if there is a wall behind it and it keeps apart from the others of its kind, made
	 * smaller (down to Shrink of its radius) where it would be seen in a cave's entrance (whose passage goes deeper
	 * than the wall): its surface the wall's (or `Before` where that is nearer, though not nearer than its radius
	 * behind its limit), its limit behind the middle of every decoration in front of it (their backs may lean on it as
	 * on the cave's wall); it is left out where it would be all hidden in the wall.
	 */
	void Add(FUghRockPiece Piece, const FUghGround& Ground, const TArray<FUghDecoration>& Decorations,
		TArray<FUghRockPiece>& Pieces, TOptional<double> Before = {})
	{
		const double Smallest = Piece.Radius * Shrink;
		while (Ground.AtEntrance(UghRockDressing::ScreenBox(Piece)) && Piece.Radius > Smallest)
		{
			Piece.Radius *= ShrinkStep;
		}
		const bool bCrowded = Pieces.ContainsByPredicate([&](const FUghRockPiece& Other)
		{
			return Other.Kind == Piece.Kind &&
				FVector2D::Distance(FVector2D(Other.X, Other.Y), FVector2D(Piece.X, Piece.Y)) <
				(Other.Radius + Piece.Radius) * Apart;
		});
		// none where the rock is already there in front of it (the mask's rock, the cliff closing beyond the screen)
		const TOptional<double> Wall = Ground.Wall(Piece.X, Piece.Y, UghRockDressing::BackFront);
		if (!Wall || bCrowded || Ground.AtEntrance(UghRockDressing::ScreenBox(Piece)))
		{
			return;
		}
		Piece.Limit = UghRockDressing::BackFront;
		for (const FUghDecoration& Decoration : Decorations)
		{
			if (UghRockDressing::InFrontOf(Decoration, Piece))
			{
				Piece.Limit = FMath::Max(Piece.Limit, Decoration.Depth + DecorationGap);
			}
		}
		// in front of what is before the wall, but its radius behind its limit at least (it may sink into that)
		Piece.Surface = Before ? FMath::Min(*Wall, FMath::Max(*Before, Piece.Limit + Piece.Radius *
			UghShapes::UnitsPerPixel)) : *Wall;
		if (Piece.Limit < Piece.Surface)
		{
			Pieces.Add(Piece);
		}
	}

	void AddCliffs(const FUghGround& Ground, const TArray<FUghDecoration>& Decorations, FRandomStream& Random,
		TArray<FUghRockPiece>& Pieces)
	{
		for (int32 Y = CliffStep / 2; Y < UghShapes::ScreenHeight + CliffStep; Y += CliffStep)
		{
			for (int32 X = CliffStep / 2 - CliffStep; X < UghShapes::ScreenWidth + CliffStep; X += CliffStep)
			{
				FUghRockPiece Piece;
				Piece.Kind = EKind::Cliff;
				Piece.X = X + Random.FRandRange(-CliffStep, CliffStep) / 2.0;
				Piece.Y = Y + Random.FRandRange(-CliffStep, CliffStep) / 2.0;
				Piece.Radius = Random.FRandRange(CliffMin, CliffMax);
				Piece.Out = Random.FRandRange(CliffOutMin, CliffOutMax);
				// its face towards the camera, give or take, on its side (the scans' upright columns lie as beds of
				// limestone; their flat foot is not seen as a shelf)
				const FQuat Side(FVector::YAxisVector, FMath::DegreesToRadians(Random.RandBool() ? 90.0 : -90.0));
				Piece.Rotation = (FQuat(FRotator(Random.FRandRange(-CliffTilt, CliffTilt),
					Random.FRandRange(-CliffTurn, CliffTurn), Random.FRandRange(-CliffTilt, CliffTilt))) * Side).Rotator();
				Piece.Variant = Random.RandHelper(MAX_int16);
				Add(Piece, Ground, Decorations, Pieces);
			}
		}
	}

	void AddRoots(const FUghGround& Ground, const TArray<FUghDecoration>& Decorations, FRandomStream& Random,
		TArray<FUghRockPiece>& Pieces)
	{
		for (int32 Y = 1; Y < UghShapes::ScreenHeight; ++Y)
		{
			for (int32 X = RootStep / 2; X < UghShapes::ScreenWidth; X += RootStep)
			{
				if (!Ground.Solid(X, Y - 1) || Ground.Solid(X, Y) || Random.RandHelper(RootEvery) != 0)
				{
					continue;
				}
				int32 Room = 0;
				while (Y + Room < UghShapes::ScreenHeight && !Ground.Solid(X, Y + Room))
				{
					++Room;
				}
				if (Room < RootRoom)
				{
					continue;
				}
				FUghRockPiece Piece;
				Piece.Kind = EKind::Root;
				Piece.Radius = Random.FRandRange(RootMin, FMath::Min(RootMax, Room * RootReach));
				Piece.X = X + Random.FRandRange(-RootStep, RootStep) / 2.0;
				Piece.Y = Y + Piece.Radius * 0.7;   // its trunk's end in the ceiling
				Piece.Out = Random.FRandRange(RootOutMin, RootOutMax);
				Piece.Rotation = FRotator(Random.FRandRange(-RootTilt, RootTilt), Random.FRandRange(-180, 180), RootHang);
				Piece.Variant = Random.RandHelper(MAX_int16);
				// out of the cliff in front of the wall there, if any: in front of it
				Add(Piece, Ground, Decorations, Pieces, CliffFront(Pieces, Piece.X, Piece.Y));
			}
		}
	}
}

double FUghRockPiece::Nearest() const
{
	return FMath::Max(Surface - Out * 2 * Radius * UghShapes::UnitsPerPixel, Limit);
}

FBox2D UghRockDressing::ScreenBox(const FUghRockPiece& Piece)
{
	const double Reach = Piece.Radius * ModelReach;
	return FBox2D(FVector2D(Piece.X - Reach, Piece.Y - Reach), FVector2D(Piece.X + Reach, Piece.Y + Reach));
}

bool UghRockDressing::InFrontOf(const FUghDecoration& Decoration, const FUghRockPiece& Piece)
{
	const FBox2D Box = ScreenBox(Piece);
	return !Decoration.Hangs() && Decoration.Height > UghDecorations::GroundCover &&
		Decoration.Left() < Box.Max.X && Box.Min.X < Decoration.Right() && Decoration.Top() < Box.Max.Y &&
		Box.Min.Y < Decoration.Bottom();
}

TArray<FUghRockPiece> UghRockDressing::Plan(const ugh_logic* Logic, const FUghRockField& Field, int32 LevelId,
	const TArray<FUghDecoration>& Decorations)
{
	TArray<FUghRockPiece> Pieces;
	if (Field.IsEmpty())
	{
		return Pieces;
	}
	const FUghGround Ground(Logic, Field);
	FRandomStream Cliffs(LevelId * 131 + 1), Roots(LevelId * 131 + 2);
	AddCliffs(Ground, Decorations, Cliffs, Pieces);
	AddRoots(Ground, Decorations, Roots, Pieces);
	return Pieces;
}
