#include "UghTurf.h"

#include "UghCavePortals.h"
#include "UghRockField.h"
#include "UghShapes.h"
#include "UghStreams.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;
	/** A card's points down its length after the root and the lip (shares of its length). */
	constexpr double Along[] = { 0.25, 0.5, 0.75, 1.0 };
	/** Pixels: a card stays this far inside the run of its edge. */
	constexpr double Inset = 0.05;
	/** Trodden this much (of 1) a card is not there at all; how much shorter a trodden one hangs. */
	constexpr double BareFrom = 0.8, TroddenShorter = 0.7;

	double Hash(double X, double Y)
	{
		return FMath::Frac(FMath::Sin(X * 12.9898 + Y * 78.233) * 43758.5453);
	}
}

bool FUghTurf::Edge(const ugh_logic* Logic, int32 X, int32 Y)
{
	return X >= 0 && X < Width && Y >= 1 && Y < Height && ugh_logic_solid(Logic, X, Y) && !ugh_logic_solid(Logic, X, Y - 1);
}

uint8 FUghTurf::PathAt(int32 X, int32 Y) const
{
	return X >= 0 && X < Width && Y >= 0 && Y < Height && !Paths.IsEmpty() ? Paths[Y * Width + X] : 0;
}

TArray<uint8> FUghTurf::MakePaths(const ugh_logic* Logic, TConstArrayView<FUghCavePortal> Portals)
{
	TArray<uint8> Mask;
	Mask.SetNumZeroed(Width * Height);
	for (const FUghCavePortal& Portal : Portals)
	{
		const int32 Y = FMath::RoundToInt32(Portal.Floor);
		const int32 Middle = FMath::FloorToInt32(Portal.X - 0.5);
		if (!Edge(Logic, Middle, Y))
		{
			continue;
		}
		for (const int32 Direction : { -1, 1 })
		{
			// here shorter, there longer
			const double Reach = PathCore + PathFade * (0.45 + 0.55 * Hash(Portal.X, Y + Direction * 0.5));
			for (int32 X = Middle; Edge(Logic, X, Y); X += Direction)
			{
				const double Distance = FMath::Abs(X + 0.5 - Portal.X);
				if (Distance >= Reach)
				{
					break;
				}
				const double Strength = Distance <= PathCore ? 1
					: 1 - FMath::SmoothStep(0.0, 1.0, (Distance - PathCore) / (Reach - PathCore));
				for (int32 Row = FMath::Max(Y - PathAbove, 0); Row <= FMath::Min(Y + PathBelow, Height - 1); ++Row)
				{
					const double Fall = Row <= Y ? 1 : 1 - double(Row - Y) / (PathBelow + 1);
					uint8& Value = Mask[Row * Width + X];
					Value = FMath::Max(Value, uint8(FMath::RoundToInt32(255 * Strength * Fall)));
				}
			}
		}
	}
	return Mask;
}

void FUghTurf::Build(const ugh_logic* Logic, const FUghRockField& Field, int32 WaterRow,
	TConstArrayView<FUghStream> Streams)
{
	Paths = MakePaths(Logic, Field.GetPortals());
	Blades = FUghRockMesh();
	Cards = 0;
	for (int32 Y = 1; Y < FMath::Min(WaterRow, Height); ++Y)
	{
		for (int32 First = 0; First < Width;)
		{
			if (!Edge(Logic, First, Y))
			{
				++First;
				continue;
			}
			int32 Last = First;
			while (Edge(Logic, Last, Y))
			{
				++Last;
			}
			for (double X = First + CardWidth / 2 + Inset; X <= Last - CardWidth / 2 - Inset;
				X += CardStep * (0.75 + 0.5 * Hash(X, Y)))
			{
				const int32 Left = FMath::FloorToInt32(X - CardWidth / 2), Right = FMath::FloorToInt32(X + CardWidth / 2);
				// a stream pours over the edge here
				if (Streams.ContainsByPredicate([&](const FUghStream& Stream)
				{
					return Stream.Y == Y && X + CardWidth / 2 > Stream.Left - 1 && X - CardWidth / 2 < Stream.Right + 1;
				}))
				{
					continue;
				}
				double Trodden = 0;
				for (int32 Column = Left; Column <= Right; ++Column)
				{
					Trodden = FMath::Max(Trodden, PathAt(Column, Y) / 255.0);
				}
				if (Trodden >= BareFrom)
				{
					continue;
				}
				// as long as the edge's long tufts say, shorter where trodden; only as far down as rock is under it
				const double Tuft = 0.5 + 0.5 * FMath::PerlinNoise1D(X * 0.31 + Y * 1.7);
				double Length = FMath::Lerp(Shortest, Longest, FMath::Clamp(0.9 * Tuft + 0.55 * Hash(Y, X) - 0.25, 0.0, 1.0)) *
					(1 - TroddenShorter * Trodden);
				int32 Room = 0;
				while (Room < FMath::CeilToInt32(Length) && Y + Room < Height)
				{
					bool bSolid = true;
					for (int32 Column = Left; Column <= Right; ++Column)
					{
						bSolid &= ugh_logic_solid(Logic, Column, Y + Room) != 0;
					}
					if (!bSolid)
					{
						break;
					}
					++Room;
				}
				Length = FMath::Min(Length, Room - 0.05);
				if (Length >= 0.8)
				{
					AddCard(Field, X, Y, Length, Hash(X + 0.3, Y), Trodden);
				}
			}
			First = Last;
		}
	}
}

void FUghTurf::AddCard(const FUghRockField& Field, double X, int32 Y, double Length, double Seed, double Trodden)
{
	// the face's front at row y under the card (the frontmost of its middle and sides), pixels of depth
	auto Front = [&Field, X](double Row)
	{
		double Nearest = UghShapes::PlaneThickness / 2 / UghShapes::UnitsPerPixel;
		for (const double Across : { -CardWidth / 2 + 0.2, 0.0, CardWidth / 2 - 0.2 })
		{
			for (double Depth = FUghRockField::FrontDepth - 1; Depth < Nearest; Depth += 0.25)
			{
				if (Field.Sample(FVector(X + Across, Row, Depth)) > 0)
				{
					Nearest = Depth;
					break;
				}
			}
		}
		return Nearest;
	};
	// root on the top, the lip at the face's front, then hanging down it, further out towards the tip
	TArray<FVector, TInlineAllocator<8>> Points;   // y down from the edge's row, depth (pixels); x is the card's
	const double Lip = FMath::Min(Front(Y + 0.5) - Off, RootDepth - 0.3);
	Points.Add(FVector(0, Y, FMath::Min(RootDepth, Lip + RootBehind)));
	Points.Add(FVector(0, Y, Lip));
	for (const double Share : Along)
	{
		const double Down = FMath::Max(Length * Share, 0.3);
		Points.Add(FVector(0, Y + Down, Front(Y + FMath::Min(Down, Length - 0.5)) - FMath::Lerp(Off, TipOff, Share)));
	}
	const FVector Normal = FVector(0, 0.85, 0.55).GetSafeNormal();   // up and towards the camera: lit as the tops
	const int32 Base = Blades.Vertices.Num();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const double V = Index == 0 ? 0 : Index == 1 ? 0.12 : 0.12 + 0.88 * Along[Index - 2];
		for (const int32 Side : { 0, 1 })
		{
			Blades.Vertices.Add(UghShapes::ToWorld(X + (Side - 0.5) * CardWidth, Points[Index].Y,
				Points[Index].Z * UghShapes::UnitsPerPixel));
			Blades.Normals.Add(Normal);
			Blades.UVs.Add(FVector2D(Side, V));
			Blades.Colors.Add(FColor(uint8(255 * Seed), uint8(255 * V), uint8(255 * Trodden), 255));
		}
	}
	for (int32 Index = 0; Index + 1 < Points.Num(); ++Index)
	{
		const int32 A = Base + Index * 2;
		Blades.Triangles.Append({ A, A + 2, A + 1, A + 1, A + 2, A + 3 });
	}
	++Cards;
}
