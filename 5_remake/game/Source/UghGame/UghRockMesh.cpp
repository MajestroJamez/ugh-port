#include "UghRockMesh.h"

#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/** A run of pixels in a row or column: First .. Last - 1. */
	struct FRun
	{
		int32 First, Last;
		bool operator==(const FRun& Other) const { return First == Other.First && Last == Other.Last; }
	};

	bool Solid(const ugh_logic* Logic, int32 X, int32 Y) { return ugh_logic_solid(Logic, X, Y) != 0; }

	/** The runs of 0 .. Count - 1 where `Wanted` holds. */
	template <typename TWanted>
	TArray<FRun> Runs(int32 Count, TWanted Wanted)
	{
		TArray<FRun> Result;
		for (int32 I = 0; I < Count;)
		{
			if (!Wanted(I))
			{
				++I;
				continue;
			}
			const int32 First = I;
			while (I < Count && Wanted(I))
			{
				++I;
			}
			Result.Add({ First, I });
		}
		return Result;
	}
}

void FUghRockMesh::Build(const ugh_logic* Logic)
{
	Vertices.Reset();
	Normals.Reset();
	UVs.Reset();
	Triangles.Reset();
	if (ugh_logic_pad_count(Logic) == 0)
	{
		return;   // no level yet
	}
	AddCut(Logic);
	AddFloorsAndCeilings(Logic);
	AddWalls(Logic);
	AddBackWall();
}

void FUghRockMesh::AddQuad(const FVector (&Corners)[4], const FVector2D (&Pixels)[4], const FVector& Normal)
{
	const int32 First = Vertices.Num();
	for (int32 I = 0; I < 4; ++I)
	{
		Vertices.Add(Corners[I]);
		Normals.Add(Normal);
		UVs.Add(FVector2D(Pixels[I].X / Width, Pixels[I].Y / Height));
	}
	// the engine draws a triangle whose corners turn clockwise seen from its front
	const bool bClockwise = (FVector::CrossProduct(Corners[1] - Corners[0], Corners[2] - Corners[0]) | Normal) < 0;
	const int32 B = bClockwise ? 1 : 3, D = bClockwise ? 3 : 1;
	Triangles.Append({ First, First + B, First + 2, First, First + 2, First + D });
}

/** The solid pixels at the front of the slab: runs of a row, merged with the same runs of the rows below. */
void FUghRockMesh::AddCut(const ugh_logic* Logic)
{
	const FVector Front = UghShapes::ToWorld(0, 0, CutDepth - 1) - UghShapes::ToWorld(0, 0, CutDepth);
	auto Rectangle = [&](const FRun& Run, int32 Top, int32 Bottom)
	{
		const FVector2D Pixels[4] = { { double(Run.First), double(Top) }, { double(Run.Last), double(Top) },
			{ double(Run.Last), double(Bottom) }, { double(Run.First), double(Bottom) } };
		FVector Corners[4];
		for (int32 I = 0; I < 4; ++I)
		{
			Corners[I] = UghShapes::ToWorld(Pixels[I].X, Pixels[I].Y, CutDepth);
		}
		AddQuad(Corners, Pixels, Front.GetSafeNormal());
	};
	TArray<FRun> Open;
	TArray<int32> OpenTop;
	for (int32 Y = 0; Y <= Height; ++Y)
	{
		const TArray<FRun> Row = Y < Height ? Runs(Width, [&](int32 X) { return Solid(Logic, X, Y); }) : TArray<FRun>();
		TArray<int32> Top;
		for (const FRun& Run : Row)
		{
			const int32 Continued = Open.Find(Run);
			Top.Add(Continued == INDEX_NONE ? Y : OpenTop[Continued]);
		}
		for (int32 I = 0; I < Open.Num(); ++I)
		{
			if (!Row.Contains(Open[I]))
			{
				Rectangle(Open[I], OpenTop[I], Y);
			}
		}
		Open = Row;
		OpenTop = Top;
	}
}

/** Between two rows: a floor where rock is below and air above, a ceiling where it is the other way round. */
void FUghRockMesh::AddFloorsAndCeilings(const ugh_logic* Logic)
{
	const FVector Up = (UghShapes::ToWorld(0, -1, 0) - UghShapes::ToWorld(0, 0, 0)).GetSafeNormal();
	for (int32 Y = 0; Y <= Height; ++Y)
	{
		for (const bool bFloor : { true, false })
		{
			// a floor faces up, at the top of the rock pixel; its colour is that pixel's
			const int32 RockRow = bFloor ? Y : Y - 1, AirRow = bFloor ? Y - 1 : Y;
			const double ColourRow = RockRow + 0.5;
			for (const FRun& Run : Runs(Width, [&](int32 X) { return Solid(Logic, X, RockRow) && !Solid(Logic, X, AirRow); }))
			{
				const FVector Corners[4] = { UghShapes::ToWorld(Run.First, Y, CutDepth),
					UghShapes::ToWorld(Run.Last, Y, CutDepth), UghShapes::ToWorld(Run.Last, Y, BackDepth),
					UghShapes::ToWorld(Run.First, Y, BackDepth) };
				const FVector2D Pixels[4] = { { double(Run.First), ColourRow }, { double(Run.Last), ColourRow },
					{ double(Run.Last), ColourRow }, { double(Run.First), ColourRow } };
				AddQuad(Corners, Pixels, bFloor ? Up : -Up);
			}
		}
	}
}

/** Between two columns: a wall facing the air on its side. */
void FUghRockMesh::AddWalls(const ugh_logic* Logic)
{
	const FVector Right = (UghShapes::ToWorld(1, 0, 0) - UghShapes::ToWorld(0, 0, 0)).GetSafeNormal();
	for (int32 X = 0; X <= Width; ++X)
	{
		for (const bool bFacingRight : { true, false })
		{
			const int32 RockColumn = bFacingRight ? X - 1 : X, AirColumn = bFacingRight ? X : X - 1;
			const double ColourColumn = RockColumn + 0.5;
			for (const FRun& Run : Runs(Height, [&](int32 Y) { return Solid(Logic, RockColumn, Y) && !Solid(Logic, AirColumn, Y); }))
			{
				const FVector Corners[4] = { UghShapes::ToWorld(X, Run.First, CutDepth),
					UghShapes::ToWorld(X, Run.Last, CutDepth), UghShapes::ToWorld(X, Run.Last, BackDepth),
					UghShapes::ToWorld(X, Run.First, BackDepth) };
				const FVector2D Pixels[4] = { { ColourColumn, double(Run.First) }, { ColourColumn, double(Run.Last) },
					{ ColourColumn, double(Run.Last) }, { ColourColumn, double(Run.First) } };
				AddQuad(Corners, Pixels, bFacingRight ? Right : -Right);
			}
		}
	}
}

double FUghRockMesh::BackWallDepth(double X, double Y)
{
	const double Bumps = 0.5 + 0.35 * FMath::PerlinNoise2D(FVector2D(X, Y) * 0.06) +
		0.15 * FMath::PerlinNoise2D(FVector2D(X, Y) * 0.2);
	return BackDepth - BumpDepth * FMath::Clamp(Bumps, 0.0, 1.0);   // only forward: it meets every wall of the rock
}

/** Behind the whole screen: a grid with the bumps of BackWallDepth, each cell a quad facing the camera's way. */
void FUghRockMesh::AddBackWall()
{
	auto At = [](double X, double Y) { return UghShapes::ToWorld(X, Y, BackWallDepth(X, Y)); };
	for (int32 Y = 0; Y < Height; Y += WallStep)
	{
		for (int32 X = 0; X < Width; X += WallStep)
		{
			const double X1 = X + WallStep, Y1 = Y + WallStep;
			const FVector Corners[4] = { At(X, Y), At(X1, Y), At(X1, Y1), At(X, Y1) };
			const FVector2D Pixels[4] = { { double(X), double(Y) }, { X1, double(Y) }, { X1, Y1 }, { double(X), Y1 } };
			const FVector Normal = FVector::CrossProduct(Corners[2] - Corners[0], Corners[3] - Corners[1]).GetSafeNormal();
			const FVector Front = UghShapes::ToWorld(0, 0, -1) - UghShapes::ToWorld(0, 0, 0);
			AddQuad(Corners, Pixels, (Normal | Front) >= 0 ? Normal : -Normal);
		}
	}
}
