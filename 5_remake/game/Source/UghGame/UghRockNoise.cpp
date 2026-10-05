#include "UghRockNoise.h"

namespace
{
	/** How wavy the blocks' borders are (cells) and how fine their waves (per pixel). */
	constexpr double Wave = 0.22, WaveScale = 0.08;
	/** The crack between two blocks is about this wide (cells); a block's face tilts at most this much (per cell). */
	constexpr double CrackWidth = 0.08, Tilt = 0.6;
	/** This share of the borders between blocks are open cracks; the others only the step from block to block. */
	constexpr double CrackShare = 0.55;

	/** A number 0 .. 1 of cell I, J (and a salt) that looks random. */
	double Fraction(int32 I, int32 J, uint32 Salt)
	{
		uint32 H = uint32(I) * 0x8da6b343u ^ uint32(J) * 0xd8163841u ^ Salt * 0xcb1ab31fu;
		H ^= H >> 15;
		H *= 0x2c1b3c6du;
		H ^= H >> 12;
		H *= 0x297a2d39u;
		H ^= H >> 15;
		return (H & 0xffffff) / double(0xffffff);
	}
}

UghRockNoise::FBlock UghRockNoise::Blocks(double X, double Y, double Width, double Height, uint32 Salt)
{
	// the cells' coordinates, wavy; the nearest and the second nearest of their jittered middles (Voronoi)
	const double U = X / Width + Wave * FMath::PerlinNoise2D(FVector2D(X, Y) * WaveScale);
	const double V = Y / Height + Wave * FMath::PerlinNoise2D(FVector2D(X + 317, Y - 211) * WaveScale);
	const int32 CI = FMath::FloorToInt32(U), CJ = FMath::FloorToInt32(V);
	double Nearest = UE_BIG_NUMBER, Second = UE_BIG_NUMBER;
	FIntPoint Cell(CI, CJ), Neighbour(CI, CJ);
	FVector2D Middle(U, V);
	for (int32 J = CJ - 1; J <= CJ + 1; ++J)
	{
		for (int32 I = CI - 1; I <= CI + 1; ++I)
		{
			const FVector2D Point(I + 0.15 + 0.7 * Fraction(I, J, Salt), J + 0.15 + 0.7 * Fraction(I, J, Salt + 1));
			const double Distance = FVector2D::Distance(Point, FVector2D(U, V));
			if (Distance < Nearest)
			{
				Second = Nearest;
				Neighbour = Cell;
				Nearest = Distance;
				Cell = FIntPoint(I, J);
				Middle = Point;
			}
			else if (Distance < Second)
			{
				Second = Distance;
				Neighbour = FIntPoint(I, J);
			}
		}
	}
	FBlock Block;
	const FVector2D Slope(Fraction(Cell.X, Cell.Y, Salt + 3) * 2 - 1, Fraction(Cell.X, Cell.Y, Salt + 4) * 2 - 1);
	Block.Height = FMath::Clamp(Fraction(Cell.X, Cell.Y, Salt + 2) + Tilt * ((FVector2D(U, V) - Middle) | Slope),
		0.0, 1.0);
	// half the difference of the two distances: about how far the border between the two cells is; not every border
	// is open (the same for both cells)
	const bool bOpen = Fraction(Cell.X + Neighbour.X, Cell.Y + Neighbour.Y, Salt + 5) < CrackShare;
	Block.Crack = bOpen ? FMath::SmoothStep(0.0, CrackWidth, (Second - Nearest) / 2) : 1;
	return Block;
}

double UghRockNoise::SmoothMax(double A, double B, double K)
{
	const double H = FMath::Max(K - FMath::Abs(A - B), 0.0) / K;
	return FMath::Max(A, B) + H * H * K / 4;
}
