#include "UghSurfaceNets.h"

#include "UghRockField.h"
#include "UghStackField.h"

namespace
{
	/** The corners of a cell, bit 0 along x, bit 1 along y, bit 2 along the depth; its edges as pairs of them. */
	constexpr int32 Edges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
}

template <class TGrid>
void UghSurfaceNets::Build(const TGrid& Grid, TArray<FVector>& OutPoints, TArray<FUghNetQuad>& OutQuads)
{
	OutPoints.Reset();
	OutQuads.Reset();
	constexpr int32 NX = TGrid::Columns, NY = TGrid::Rows;
	const int32 NZ = TGrid::Layers();
	auto CellIndex = [](int32 I, int32 J, int32 K) { return (K * (NY - 1) + J) * (NX - 1) + I; };
	TArray<int32> CellPoints;
	CellPoints.Init(INDEX_NONE, (NX - 1) * (NY - 1) * (NZ - 1));

	// a point in every cell with corners on both sides
	for (int32 K = 0; K + 1 < NZ; ++K)
	{
		for (int32 J = 0; J + 1 < NY; ++J)
		{
			for (int32 I = 0; I + 1 < NX; ++I)
			{
				float Values[8];
				int32 Solid = 0;
				for (int32 Corner = 0; Corner < 8; ++Corner)
				{
					Values[Corner] = Grid.At(I + (Corner & 1), J + (Corner >> 1 & 1), K + (Corner >> 2));
					Solid += Values[Corner] > 0;
				}
				if (Solid == 0 || Solid == 8)
				{
					continue;
				}
				FVector Sum = FVector::ZeroVector;
				int32 Crossings = 0;
				for (const auto& [A, B] : Edges)
				{
					if ((Values[A] > 0) != (Values[B] > 0))
					{
						const FVector From = TGrid::Node(I + (A & 1), J + (A >> 1 & 1), K + (A >> 2));
						const FVector To = TGrid::Node(I + (B & 1), J + (B >> 1 & 1), K + (B >> 2));
						Sum += FMath::Lerp(From, To, double(Values[A] / (Values[A] - Values[B])));
						++Crossings;
					}
				}
				CellPoints[CellIndex(I, J, K)] = OutPoints.Add(Sum / Crossings);
			}
		}
	}

	// a quad across every crossed edge of the grid that has four cells around it
	const int32 Size[3] = { NX, NY, NZ };
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		const int32 U = (Axis + 1) % 3, V = (Axis + 2) % 3;   // the other two axes
		for (int32 K = 0; K < NZ; ++K)
		{
			for (int32 J = 0; J < NY; ++J)
			{
				for (int32 I = 0; I < NX; ++I)
				{
					int32 Node0[3] = { I, J, K };
					if (Node0[Axis] + 1 >= Size[Axis] || Node0[U] < 1 || Node0[U] + 1 >= Size[U] || Node0[V] < 1 ||
						Node0[V] + 1 >= Size[V])
					{
						continue;
					}
					int32 Node1[3] = { I, J, K };
					++Node1[Axis];
					const bool bSolid0 = Grid.At(Node0[0], Node0[1], Node0[2]) > 0;
					if (bSolid0 == (Grid.At(Node1[0], Node1[1], Node1[2]) > 0))
					{
						continue;
					}
					// the cells around the edge, in order around it
					FUghNetQuad Quad;
					const int32 Around[4][2] = { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } };
					for (int32 Corner = 0; Corner < 4; ++Corner)
					{
						int32 Cell[3] = { I, J, K };
						Cell[U] += Around[Corner][0];
						Cell[V] += Around[Corner][1];
						Quad.Corners[Corner] = CellPoints[CellIndex(Cell[0], Cell[1], Cell[2])];
					}
					Quad.Outward = FVector::ZeroVector;
					Quad.Outward[Axis] = bSolid0 ? 1 : -1;
					OutQuads.Add(Quad);
				}
			}
		}
	}
}

template void UghSurfaceNets::Build<FUghRockField>(const FUghRockField&, TArray<FVector>&, TArray<FUghNetQuad>&);
template void UghSurfaceNets::Build<FUghStackField>(const FUghStackField&, TArray<FVector>&, TArray<FUghNetQuad>&);
