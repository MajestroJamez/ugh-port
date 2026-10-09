// The rock of every level as an automation test of the editor (Ugh.Rock): its edge in the plane of the play, its
// face in front of it never over the air.
#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/BinarySearch.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghJson.h"
#include "UghGround.h"
#include "UghLevelArt.h"
#include "UghPadSigns.h"
#include "UghRockField.h"
#include "UghRockMesh.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghStreams.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;
	/** Where the mesh is cut: the plane of the play and a pixel in front of and behind it (units). */
	constexpr double CutDepths[] = { -UghShapes::UnitsPerPixel, 0, UghShapes::UnitsPerPixel };
	/** Points checked in a pixel besides its centre: Samples x Samples. */
	constexpr int32 Samples = 4;
	/**
	 * How far (pixels) a point off a pixel's centre may lie from a pixel of the kind the cut gives it: the cut leaves
	 * the pixel steps' corners (half a pixel), every pixel's centre is on its side.
	 */
	constexpr double Tolerance = 0.5;

	/** A piece of the cut through the mesh, pixels, and the way from the rock to the air across it. */
	struct FPiece
	{
		FVector2D A, B;
		FVector2D Outward;
	};

	/** The mesh cut at `Depth` (units): pieces of its surface in the plane. */
	TArray<FPiece> Cut(const FUghRockMesh& Mesh, double Depth)
	{
		TArray<FPiece> Pieces;
		const double Plane = UghShapes::ToWorld(0, 0, Depth).Y;
		auto Pixel = [](const FVector& Point)
		{
			return FVector2D(Point.X / UghShapes::UnitsPerPixel, Height - Point.Z / UghShapes::UnitsPerPixel);
		};
		for (int32 T = 0; T < Mesh.Triangles.Num(); T += 3)
		{
			const FVector V[3] = { Mesh.Vertices[Mesh.Triangles[T]], Mesh.Vertices[Mesh.Triangles[T + 1]],
				Mesh.Vertices[Mesh.Triangles[T + 2]] };
			TArray<FVector2D, TInlineAllocator<2>> Ends;
			for (int32 E = 0; E < 3; ++E)
			{
				const FVector& P = V[E];
				const FVector& Q = V[(E + 1) % 3];
				if ((P.Y < Plane) != (Q.Y < Plane))
				{
					Ends.Add(Pixel(FMath::Lerp(P, Q, (Plane - P.Y) / (Q.Y - P.Y))));
				}
			}
			if (Ends.Num() == 2)
			{
				// the engine's front is where the corners turn clockwise: the outside
				const FVector Outward = -FVector::CrossProduct(V[1] - V[0], V[2] - V[0]);
				Pieces.Add({ Ends[0], Ends[1], FVector2D(Outward.X, -Outward.Z) });
			}
		}
		return Pieces;
	}

	/** The cut, its pieces indexed by the rows of pixels they pass, to find quickly what a row crosses. */
	class FCut
	{
	public:
		explicit FCut(TArray<FPiece> InPieces) : Pieces(MoveTemp(InPieces))
		{
			Rows.SetNum(Height + 2 * Beyond);
			for (int32 Index = 0; Index < Pieces.Num(); ++Index)
			{
				const double A = Pieces[Index].A.Y, B = Pieces[Index].B.Y;
				for (int32 Row = Bucket(FMath::Min(A, B)); Row <= Bucket(FMath::Max(A, B)); ++Row)
				{
					Rows[Row].Add(Index);
				}
			}
		}

		/**
		 * Whether point P is in the rock: the side of the nearest piece its row crosses. A row that crosses none is in
		 * the rock: the rock closes at the rim of its grid (FUghRockField), beyond the screen.
		 */
		bool RockAt(const FVector2D& P) const
		{
			const TArray<FCrossing>& Line = Crossings(P.Y);
			if (Line.IsEmpty())
			{
				return true;
			}
			const int32 Before = Algo::UpperBoundBy(Line, P.X, &FCrossing::X) - 1;
			return Before >= 0 ? Line[Before].bRockAfter : !Line[0].bRockAfter;
		}

	private:
		/** Where a row crosses a piece, and whether going on to the right enters the rock. */
		struct FCrossing
		{
			double X;
			bool bRockAfter;
		};
		/** How far beyond the screen the pieces may lie, pixels. */
		static constexpr int32 Beyond = 64;

		int32 Bucket(double Y) const
		{
			return FMath::Clamp(FMath::FloorToInt32(Y) + Beyond, 0, Rows.Num() - 1);
		}

		/** The crossings of the row at `Y`, from the left (the last row's are kept). */
		const TArray<FCrossing>& Crossings(double Y) const
		{
			if (LastY == Y)
			{
				return Last;
			}
			LastY = Y;
			Last.Reset();
			for (const int32 Index : Rows[Bucket(Y)])
			{
				const FPiece& Piece = Pieces[Index];
				if ((Piece.A.Y <= Y) != (Piece.B.Y <= Y))
				{
					// passing the piece to the right enters the rock when its outside faces left
					const double T = (Y - Piece.A.Y) / (Piece.B.Y - Piece.A.Y);
					Last.Add({ FMath::Lerp(Piece.A.X, Piece.B.X, T), Piece.Outward.X < 0 });
				}
			}
			Last.Sort([](const FCrossing& L, const FCrossing& R) { return L.X < R.X; });
			return Last;
		}

		TArray<FPiece> Pieces;
		TArray<TArray<int32>> Rows;
		mutable double LastY = -1e9;
		mutable TArray<FCrossing> Last;
	};

	/** How far point P is from the nearest pixel (around its own) of the kind `bRock`; 99 when none is near. */
	double Off(const ugh_logic* Logic, const FVector2D& P, bool bRock)
	{
		double Nearest = 99;
		const int32 X = FMath::FloorToInt32(P.X), Y = FMath::FloorToInt32(P.Y);
		for (int32 NY = FMath::Max(Y - 1, 0); NY <= FMath::Min(Y + 1, Height - 1); ++NY)
		{
			for (int32 NX = FMath::Max(X - 1, 0); NX <= FMath::Min(X + 1, Width - 1); ++NX)
			{
				if ((ugh_logic_solid(Logic, NX, NY) != 0) == bRock)
				{
					const double DX = FMath::Max3(NX - P.X, P.X - NX - 1, 0.0);
					const double DY = FMath::Max3(NY - P.Y, P.Y - NY - 1, 0.0);
					Nearest = FMath::Min(Nearest, FMath::Sqrt(DX * DX + DY * DY));
				}
			}
		}
		return Nearest;
	}

	/**
	 * How many nodes of the field in front of the slab of the play are rock over a pixel of air of the mask (the face
	 * covering the play: its edge may break off inside the mask, never reach over it).
	 */
	int32 OverTheAir(const ugh_logic* Logic, const FUghRockField& Field)
	{
		int32 Over = 0;
		const TConstArrayView<double> Depths = FUghRockField::Depths();
		for (int32 K = 0; K < Depths.Num() && Depths[K] < -FUghRockField::SlabHalf; ++K)
		{
			for (int32 Y = 0; Y < Height; ++Y)
			{
				for (int32 X = 0; X < Width; ++X)
				{
					Over += ugh_logic_solid(Logic, X, Y) == 0 &&
						Field.At(X + FUghRockOutline::MarginX, Y + FUghRockOutline::MarginY, K) > 0;
				}
			}
		}
		return Over;
	}

	/**
	 * The cut against the mask: how many pixels' centres are on the wrong side, how many other points are further
	 * than the tolerance from a pixel of the kind the cut gives them, the furthest of all. Row by row of points (the
	 * cut keeps a row's crossings).
	 */
	void Check(const ugh_logic* Logic, const FCut& Section, int32& OutCentres, int32& OutPoints, double& InOutWorst)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 Row = -1; Row < Samples; ++Row)   // -1: the centres
			{
				const double PY = Y + (Row < 0 ? 0.5 : (Row + 0.5) / Samples);
				for (int32 X = 0; X < Width; ++X)
				{
					const bool bSolid = ugh_logic_solid(Logic, X, Y) != 0;
					if (Row < 0)
					{
						OutCentres += Section.RockAt(FVector2D(X + 0.5, PY)) != bSolid;
						continue;
					}
					for (int32 Column = 0; Column < Samples; ++Column)
					{
						const FVector2D P(X + (Column + 0.5) / Samples, PY);
						const bool bRock = Section.RockAt(P);
						if (bRock != bSolid)
						{
							const double Distance = Off(Logic, P, bRock);
							InOutWorst = FMath::Max(InOutWorst, Distance);
							OutPoints += Distance > Tolerance;
						}
					}
				}
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRockTest, "Ugh.Rock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRockTest::RunTest(const FString& Parameters)
{
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	FUghSimulation Simulation;
	FUghLevelArt Art;
	TSharedPtr<FJsonObject> Levels;
	FString Error;
	const FString LevelsPath = Assets / UghJson::LevelsFile;
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error) &&
		UghJson::ReadObject(LevelsPath, Levels, Error) && Art.Load(*Levels, LevelsPath, Error)))
	{
		return false;
	}
	TSet<int32> Done;
	double Worst = 0;
	for (const auto& [Players, Count] : Modes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const int32 LevelId = Simulation.GetCurrent().level_id;
			if (Done.Contains(LevelId))
			{
				continue;
			}
			Done.Add(LevelId);
			FUghRockField Field;
			Field.Build(Logic, {}, Art.Doors(LevelId));   // with its cave's entrances behind the slab
			// and the channels of its streams in front of and behind it
			const TArray<FUghPadSign> Signs = UghPadSigns::Plan(Logic, FUghGround(Logic, Field), Art.Signs(LevelId));
			Field.CarveChannels(UghStreams::Plan(Logic, Field, Simulation.GetCurrent().water_level / UghShapes::Subpixels,
				Signs));
			TestEqual(FString::Printf(TEXT("level_id %d: nodes of the face in front of the slab over the air"), LevelId),
				OverTheAir(Logic, Field), 0);
			FUghRockMesh Mesh;
			Mesh.Build(Field);
			for (const double Depth : CutDepths)
			{
				const FCut Section(Cut(Mesh, Depth));
				int32 Centres = 0, Points = 0;
				Check(Logic, Section, Centres, Points, Worst);
				const FString Name = FString::Printf(TEXT("level_id %d at depth %.0f"), LevelId, Depth);
				TestEqual(Name + TEXT(": pixel centres on the wrong side of the rock's edge"), Centres, 0);
				TestEqual(Name + TEXT(": points further than the tolerance from the rock's edge"), Points, 0);
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d levels; the edge is at most %.2f px off the pixels' borders"), Done.Num(), Worst));
	return true;
}

#endif
