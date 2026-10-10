#include "UghTreeRoots.h"

#include "Misc/FileHelper.h"
#include "UghRockField.h"
#include "UghShapes.h"
#include "UghStreams.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 RootsHeight = UghShapes::ScreenHeight;
	/** A root branches at a step with this chance (once it is this many steps long and thick enough). */
	constexpr double BranchChance = 0.07, BranchThickest = 0.75;
	constexpr int32 BranchAfter = 6;

	bool RootSolid(const ugh_logic* Logic, int32 X, int32 Y) { return ugh_logic_solid(Logic, X, Y) != 0; }

	/** The row of a ledge's surface (rock, air above it) in column `X` within `Reach` rows of `Y`; -1 when none. */
	int32 RootEdgeNear(const ugh_logic* Logic, int32 X, int32 Y, int32 Reach)
	{
		for (int32 Away = 0; Away <= Reach; ++Away)
		{
			for (const int32 Row : { Y + Away, Y - Away })
			{
				if (RootSolid(Logic, X, Row) && !RootSolid(Logic, X, Row - 1))
				{
					return Row;
				}
			}
		}
		return -1;
	}

	/** Row `Y` is rock from x - `Need` to x + `Need`. */
	bool RootClear(const ugh_logic* Logic, double X, int32 Y, double Need)
	{
		for (int32 Column = FMath::FloorToInt32(X - Need); Column <= FMath::FloorToInt32(X + Need); ++Column)
		{
			if (!RootSolid(Logic, Column, Y))
			{
				return false;
			}
		}
		return true;
	}

	/** Pixels of rock in row `Y` from x the way of `Way` before the air (at most 8). */
	int32 RootRockBeside(const ugh_logic* Logic, double X, int32 Y, int32 Way)
	{
		int32 Count = 0;
		while (Count < 8 && RootSolid(Logic, FMath::FloorToInt32(X) + Way * (Count + 1), Y))
		{
			++Count;
		}
		return Count;
	}

	/** The face's front at x, y (pixels of depth), the frontmost of a root's middle and its sides there. */
	double RootFront(const FUghRockField& Field, double X, double Y, double Radius)
	{
		double Nearest = FUghRockField::SlabHalf;
		for (const double Across : { -0.7 * Radius, 0.0, 0.7 * Radius })
		{
			for (double Depth = FUghRockField::FrontDepth - 1; Depth < Nearest; Depth += 0.25)
			{
				if (Field.Sample(FVector(X + Across, Y, Depth)) > 0)
				{
					Nearest = Depth;
					break;
				}
			}
		}
		return Nearest;
	}
}

bool FUghTreePlaces::Load(const FString& File, FString& OutError)
{
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *File))
	{
		OutError = TEXT("cannot read ") + File;
		return false;
	}
	Feet.Reset();
	int32 Level = -1;
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(TEXT("level ")))
		{
			Level = FCString::Atoi(*Line.Mid(6));
			continue;
		}
		if (Level < 0 || !Line.StartsWith(TEXT("tree ")))
		{
			continue;
		}
		TArray<FString> Words;
		Line.ParseIntoArrayWS(Words);
		TOptional<int32> X, Y;
		for (const FString& Word : Words)
		{
			if (Word.StartsWith(TEXT("x=")))
			{
				X = FCString::Atoi(*Word.Mid(2));
			}
			else if (Word.StartsWith(TEXT("y=")))
			{
				Y = FCString::Atoi(*Word.Mid(2));
			}
		}
		if (X && Y)
		{
			Feet.FindOrAdd(Level).Add(FVector2D(double(*X) / UghShapes::Subpixels + TreeWidth / 2.0,
				double(*Y) / UghShapes::Subpixels + TreeHeight));
		}
	}
	return true;
}

TArray<FVector2D> FUghTreePlaces::Of(int32 LevelId) const
{
	const TArray<FVector2D>* Found = Feet.Find(LevelId);
	return Found ? *Found : TArray<FVector2D>();
}

void FUghTreeRoots::Build(const ugh_logic* Logic, const FUghRockField& Field, TConstArrayView<FVector2D> Feet,
	int32 WaterRow, TConstArrayView<FUghStream> Streams, int32 LevelId)
{
	Strands.Reset();
	Mesh = FUghRockMesh();
	FRandomStream Random(LevelId * 977 + 13);
	const auto InStream = [Streams](double X, int32 Y, double Need)
	{
		return Streams.ContainsByPredicate([&](const FUghStream& Stream)
		{
			return Y >= Stream.Y - 1 && X + Need > Stream.Left - 2 && X - Need < Stream.Right + 2;
		});
	};
	struct FSeed
	{
		double X = 0, Y = 0, Radius = 0, Length = 0, Drift = 0;
		bool bLip = false;   // it starts at the lip of the ledge (else it branches off another root)
	};
	// a root of radius R at x, y lies on rock: Margin of it on either side, below it, its top at most at a surface
	const auto Fits = [&](double X, double Y, double Radius)
	{
		const double Need = Radius + Margin;
		for (int32 Row = FMath::FloorToInt32(Y - Radius + 0.01); Row <= FMath::FloorToInt32(Y + Need - 0.01); ++Row)
		{
			if (!RootClear(Logic, X, Row, Need))
			{
				return false;
			}
		}
		return !InStream(X, FMath::FloorToInt32(Y), Need);
	};
	TArray<FSeed> Seeds;
	for (const FVector2D& Foot : Feet)
	{
		const int32 Roots = 5 + Random.RandHelper(3);
		for (int32 Index = 0; Index < Roots; ++Index)
		{
			// across the foot, the middle ones thickest and longest, leaning outwards
			const double Side = (Index + 0.5) / Roots * 2 - 1;
			const double X = Foot.X + Side * Spread + Random.FRandRange(-1.2, 1.2);
			const int32 Edge = RootEdgeNear(Logic, FMath::FloorToInt32(X), FMath::RoundToInt32(Foot.Y), 4);
			// thinner on a thin ledge
			double Radius = FMath::Lerp(Thickest, Thinnest, FMath::Abs(Side)) * Random.FRandRange(0.85, 1.1);
			while (Edge >= 0 && Radius >= Thinnest * 0.5 && !Fits(X, Edge + Radius, Radius))
			{
				Radius *= 0.8;
			}
			if (Edge < 0 || Edge >= WaterRow - 2 || Radius < Thinnest * 0.5)
			{
				continue;
			}
			Seeds.Add({ X, Edge + Radius, Radius,
				Random.FRandRange(Shortest, Longest) * (1 - 0.35 * FMath::Abs(Side)), 0.35 * Side, true });
		}
	}
	while (!Seeds.IsEmpty())
	{
		const FSeed Seed = Seeds.Pop(EAllowShrinking::No);
		TArray<FVector2D> Path = { FVector2D(Seed.X, Seed.Y) };
		TArray<double> Radii = { Seed.Radius };
		double X = Seed.X, Y = Seed.Y, Drift = Seed.Drift;
		int32 Way = Seed.Drift < 0 ? -1 : 1;   // the way it creeps along a ledge it cannot go down from
		int32 Crept = 0;
		for (int32 Step = 1; Step <= Seed.Length; ++Step)
		{
			const double Radius = FMath::Lerp(Seed.Radius, Tip, Step / Seed.Length);
			if (Y + 1 + Radius + Margin >= FMath::Min(WaterRow - 1, RootsHeight - 1))
			{
				break;
			}
			// down, wandering, away from the air beside it
			Drift = FMath::Clamp(Drift * 0.9 + Random.FRandRange(-0.35, 0.35), -0.9, 0.9);
			const int32 Row = FMath::FloorToInt32(Y + 1);
			double Push = 0;
			Push += RootRockBeside(Logic, X, Row, -1) < Radius + Margin + 1.5 ? 0.4 : 0;
			Push -= RootRockBeside(Logic, X, Row, 1) < Radius + Margin + 1.5 ? 0.4 : 0;
			bool bFound = false;
			for (const double Shift : { 0.0, 0.5, -0.5, 1.0, -1.0, 1.5, -1.5, 2.0, -2.0 })
			{
				const double Next = X + Drift + Push + Shift;
				if (Fits(Next, Y + 1, Radius) && Fits(Next, Y, Radius))
				{
					X = Next;
					Y += 1;
					bFound = true;
					break;
				}
			}
			// else creeping along the face of a ledge over the air, the other way when it cannot
			for (int32 Turn = 0; !bFound && Turn < 2 && Crept < Creep; ++Turn)
			{
				if (Fits(X + Way, Y, Radius))
				{
					X += Way;
					++Crept;
					bFound = true;
				}
				else
				{
					Way = -Way;
				}
			}
			if (!bFound)
			{
				break;
			}
			Path.Add(FVector2D(X, Y));
			Radii.Add(Radius);
			if (Step > BranchAfter && Radius > BranchThickest && Random.FRand() < BranchChance)
			{
				Seeds.Add({ X, Y, Radius * 0.6, (Seed.Length - Step) * 0.7,
					Drift + (Random.FRand() < 0.5 ? -0.6 : 0.6), false });
			}
		}
		if (Path.Num() < 4)
		{
			continue;
		}
		// smoothed across, the depths on the face (a little sunk into it, into its hollows), the tip into the rock
		FStrand Strand;
		TArray<double> Fronts;
		for (int32 Index = 0; Index < Path.Num(); ++Index)
		{
			const double Across = Index == 0 || Index + 1 == Path.Num() ? Path[Index].X :
				(Path[Index - 1].X + 2 * Path[Index].X + Path[Index + 1].X) / 4;
			Fronts.Add(RootFront(Field, Across, Path[Index].Y, Radii[Index]));
			Strand.Points.Add(FVector(Across, Path[Index].Y, 0));
		}
		for (int32 Index = 0; Index < Path.Num(); ++Index)
		{
			double Sum = 0;
			int32 Count = 0;
			for (int32 Near = FMath::Max(Index - 2, 0); Near <= FMath::Min(Index + 2, Path.Num() - 1); ++Near)
			{
				Sum += Fronts[Near];
				++Count;
			}
			const double Radius = Radii[Index];
			const double Laid = Sum / Count - Radius + 2 * Radius * Sunk - Out;
			Strand.Points[Index].Z =
				FMath::Clamp(Laid, Fronts[Index] - Radius - 2 * Out, Fronts[Index] + 0.3 * Radius - Out);
		}
		Strand.Radii = Radii;
		Strand.Points.Last().Z += 1.2 * Radii.Last();
		if (Seed.bLip)
		{
			// out of the rock under the tree's buttresses
			Strand.Points.Insert(FVector(Seed.X, Seed.Y, Fronts[0] + 1.5 * Seed.Radius), 0);
			Strand.Radii.Insert(Seed.Radius, 0);
		}
		AddTube(Strand);
		Strands.Add(MoveTemp(Strand));
	}
}

void FUghTreeRoots::AddTube(const FStrand& Strand)
{
	const int32 Count = Strand.Points.Num();
	TArray<FVector> Line;
	for (const FVector& Point : Strand.Points)
	{
		Line.Add(UghShapes::ToWorld(Point.X, Point.Y, Point.Z * UghShapes::UnitsPerPixel));
	}
	// a frame carried along the root, starting towards the camera
	FVector Normal = UghShapes::ToWorld(0, 0, -1) - UghShapes::ToWorld(0, 0, 0);
	const int32 Base = Mesh.Vertices.Num();
	double Along = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Tangent = (Line[FMath::Min(Index + 1, Count - 1)] - Line[FMath::Max(Index - 1, 0)]).GetSafeNormal();
		// (a root comes out of the rock towards the camera: then the frame starts across it)
		const FVector Across = Normal - Tangent * (Normal | Tangent);
		Normal = Across.SizeSquared() > 1e-6 ? Across.GetSafeNormal() :
			(FVector::UpVector - Tangent * (FVector::UpVector | Tangent)).GetSafeNormal();
		const FVector Binormal = Tangent ^ Normal;
		Along += Index > 0 ? FVector::Dist(Line[Index], Line[Index - 1]) : 0;
		// the tip closes to a point
		const double Radius = (Index + 1 == Count ? 0.05 : Strand.Radii[Index]) * UghShapes::UnitsPerPixel;
		for (int32 Side = 0; Side <= Sides; ++Side)
		{
			const double Angle = UE_TWO_PI * Side / Sides;
			const FVector Around = Normal * FMath::Cos(Angle) + Binormal * FMath::Sin(Angle);
			Mesh.Vertices.Add(Line[Index] + Around * Radius);
			Mesh.Normals.Add(Around);
			Mesh.UVs.Add(FVector2D(double(Side) / Sides, Along / BarkTile));
			Mesh.Colors.Add(FColor::White);
		}
	}
	// the engine draws a triangle whose corners turn clockwise seen from its front: which way round the rings go
	const auto At = [&](int32 Ring, int32 Side) { return Base + Ring * (Sides + 1) + Side; };
	const FVector Facing = FVector::CrossProduct(Mesh.Vertices[At(0, 1)] - Mesh.Vertices[At(0, 0)],
		Mesh.Vertices[At(1, 1)] - Mesh.Vertices[At(0, 0)]);
	const bool bTurned = (Facing | (Mesh.Normals[At(0, 0)] + Mesh.Normals[At(0, 1)])) > 0;
	for (int32 Ring = 0; Ring + 1 < Count; ++Ring)
	{
		for (int32 Side = 0; Side < Sides; ++Side)
		{
			const int32 A = At(Ring, Side), B = At(Ring, Side + 1), C = At(Ring + 1, Side + 1), D = At(Ring + 1, Side);
			if (bTurned)
			{
				Mesh.Triangles.Append({ A, C, B, A, D, C });
			}
			else
			{
				Mesh.Triangles.Append({ A, B, C, A, C, D });
			}
		}
	}
}
