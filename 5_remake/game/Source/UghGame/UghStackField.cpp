#include "UghStackField.h"

#include "Async/ParallelFor.h"
#include "Math/RandomStream.h"
#include "UghRockMesh.h"
#include "UghRockNoise.h"

namespace
{
	using Field = FUghStackField;
	constexpr double MiddleX = UghShapes::ScreenWidth / 2.0;

	/**
	 * The plan of the stone (pixels): half as wide as HalfWidth, StoneDepth deep from the frame, its corners rounded
	 * FrontRound in front and BackRound behind; narrower above TaperFrom (y: above the shroud) by Taper a pixel, wider
	 * under FootY (about the sea) by FootSpread a pixel.
	 */
	constexpr double HalfWidth = 540, StoneDepth = 520, FrontRound = 90, BackRound = 220;
	constexpr double Taper = 0.15, TaperFrom = -250, FootY = 200, FootSpread = 0.45;
	/** Its top: this high (y), lower towards its sides and back by TopDrop, bumpy, its edges rounded TopRound. */
	constexpr double TopY = -700, TopDrop = 150, TopBumps = 40, TopRound = 140;
	/** The face stands out up to FaceOut pixels further than the frame, from FaceFrom to FaceTo pixels from the hollow. */
	constexpr double FaceOut = 22, FaceFrom = 12, FaceTo = 160;
	/**
	 * The relief, pixels: lumps, flutes the rain cut down it, beds of limestone (BedHeight high, each standing out at
	 * its top: ledges, overhangs), fractured blocks, a notch the waves cut at about the sea (NotchY; not in the face).
	 * On the face it goes in at most FaceIn (the shroud stays inside); within FrameWidth of the hollow it only stands
	 * out, and less (the frame). Further than ReliefReach from the plain shape it cannot change the side: not reckoned.
	 */
	constexpr double Lumps = 40, Flutes = 16, Beds = 10, BedHeight = 110, Blocks = 12, Notch = 18, NotchY = FootY - 18,
		NotchHeight = 14;
	constexpr double FaceIn = 4, FrameWidth = 50, ReliefReach = 110;
	/** How far from the surface (pixels) its openness looks (a crevice, a hollow); the grass hangs this far down. */
	constexpr double NearLook = 4, FarLook = 12, LipReach = 40;

	/**
	 * The jungle: a spot on the top this many pixels apart (jittered); a spot of the ledges and of the walls' tops one
	 * of so many of the surface's points.
	 */
	constexpr double PlantSpacing = 22;
	constexpr int32 LedgeShare = 3, CreeperShare = 4;
	/** Plants grow where the surface faces up at least this much (on the top, on a ledge); creepers hang on a wall. */
	constexpr double TopFlat = 0.8, LedgeFlat = 0.75, WallSteep = 0.3;
	/** A spot is on its surface when the field there is within this (not in a sharp crease, where its mesh cuts in). */
	constexpr double OnSurface = 2;
	/**
	 * The top is above TopBelow; the ledges with plants below it, above the sea, at least LedgesFromHole from the
	 * hollow (pixels); creepers hang from CreepersFrom to CreepersTo below the top.
	 */
	constexpr double TopBelow = TopY + 160, LedgesAboveSea = FootY - 25, LedgesFromHole = 70, CreepersFrom = 10,
		CreepersTo = 60;
	constexpr int32 PlantSeed = 1938;

	/** How far x, y (pixels) is from the hollow of the level's rock (0 in it). */
	double FromHole(double X, double Y)
	{
		const double DX = FMath::Max3(Field::HoleLeft - X, X - Field::HoleRight, 0.0);
		const double DY = FMath::Max3(Field::HoleTop - Y, Y - Field::HoleBottom, 0.0);
		return FMath::Sqrt(DX * DX + DY * DY);
	}

	/** How far outside two shapes at once (A, B: outside each; negative inside), their edge rounded `Radius`. */
	double Both(double A, double B, double Radius)
	{
		const double QA = A + Radius, QB = B + Radius;
		return FVector2D(FMath::Max(QA, 0.0), FMath::Max(QB, 0.0)).Size() + FMath::Min(FMath::Max(QA, QB), 0.0) - Radius;
	}

	/** How far outside a rectangle of `Half` sizes around 0 (negative inside), its corners rounded `Radius`. */
	double RoundedBox(const FVector2D& P, const FVector2D& Half, double Radius)
	{
		return Both(FMath::Abs(P.X) - Half.X, FMath::Abs(P.Y) - Half.Y, Radius);
	}

	/** How far the relief stands out at `P`, `Hole` pixels from the hollow, `BehindFace` pixels behind the face. */
	double Relief(const FVector& P, double Hole, double BehindFace)
	{
		const double Lump =
			Lumps * (FMath::PerlinNoise3D(P * 0.0045) + 0.45 * FMath::PerlinNoise3D(P * 0.012 + FVector(31)));
		const double Flute = Flutes * FMath::PerlinNoise3D(FVector(P.X * 0.028, P.Y * 0.0045, P.Z * 0.028));
		// down a bed it goes in, at the next one's top it stands out again (y down)
		const double Bed = FMath::Frac(P.Y / BedHeight + 0.6 * FMath::PerlinNoise2D(FVector2D(P.X, P.Z) * 0.004));
		const double Bedding = Beds * (FMath::SmoothStep(0.0, 0.08, Bed) * 2 * FMath::Square(1 - Bed) - 0.6);
		const UghRockNoise::FBlock Block = UghRockNoise::Blocks(P.X + P.Z, P.Y, 64, 30, 11);
		const double Rough = Lump + Flute + Blocks * (Block.Height - 0.5) - 0.5 * Blocks * (1 - Block.Crack);
		const double All = Rough + Bedding;
		// (no beds in the frame: their little ledges would be lines of grass around the level)
		const double Framed = FMath::Lerp(0.25 * FMath::Max(Rough, 0.0), FMath::Max(All, -FaceIn),
			FMath::SmoothStep(0.0, FrameWidth, Hole));
		const double OnFace = 1 - FMath::SmoothStep(20.0, 80.0, BehindFace);
		return FMath::Lerp(All - Notch * FMath::Exp(-FMath::Square((P.Y - NotchY) / NotchHeight)), Framed, OnFace);
	}
}

FUghStackField::FShape FUghStackField::Shape(const FVector& P)
{
	FShape Shape;
	Shape.Hole = FromHole(P.X, P.Y);
	const double Below = FMath::Max(P.Y - FootY, 0.0);
	const double Half = HalfWidth - Taper * FMath::Max(TaperFrom - P.Y, 0.0) + FootSpread * Below;
	// (the foot spreads to the front only away from the level)
	Shape.Front = FrameDepth - FaceOut * FMath::SmoothStep(FaceFrom, FaceTo, Shape.Hole) -
		FootSpread * Below * FMath::SmoothStep(40.0, 120.0, Shape.Hole);
	const double Back = FrameDepth + StoneDepth + FootSpread * Below;
	const double Mid = (Shape.Front + Back) / 2, HalfDepth = (Back - Shape.Front) / 2;
	const double U = P.X - MiddleX, W = P.Z - Mid;
	const double Plan = RoundedBox(FVector2D(U, W), FVector2D(Half, HalfDepth), W < 0 ? FrontRound : BackRound);
	Shape.Top = TopY + TopDrop * (FMath::Square(U / Half) + 0.6 * FMath::Square(W / HalfDepth)) +
		TopBumps * FMath::PerlinNoise2D(FVector2D(P.X, P.Z) * 0.006);
	Shape.Outside = Both(Plan, Shape.Top - P.Y, TopRound);
	return Shape;
}

double FUghStackField::Value(const FVector& P)
{
	const double X = P.X, Y = P.Y, Depth = P.Z;
	const FShape Plain = Shape(P);
	const double Stone = -Plain.Outside +
		(FMath::Abs(Plain.Outside) < ReliefReach ? Relief(P, Plain.Hole, Depth - Plain.Front) : 0);
	// the hollow of the level's rock: open to the front
	const double Hollow = FMath::Max3(FMath::Max(HoleLeft - X, X - HoleRight), FMath::Max(HoleTop - Y, Y - HoleBottom),
		Depth - HoleBack);
	return FMath::Min(Stone, Hollow);
}

FColor FUghStackField::Shade(const FVector& Point, const FVector& Outward)
{
	// open where the field outside keeps falling as on a flat surface, closed in a crevice
	const double Near = FMath::Clamp(-Value(Point + Outward * NearLook) / NearLook, 0.0, 1.0);
	const double Far = FMath::Clamp(-Value(Point + Outward * FarLook) / FarLook, 0.0, 1.0);
	const double Lip = 1 - FMath::SmoothStep(0.0, LipReach, Point.Y - Shape(Point).Top);
	return FColor(uint8(255 * (0.5 * Near + 0.5 * Far)), 0, uint8(255 * Lip), uint8(255 * FUghRockMesh::Patches(Point)));
}

FVector FUghStackField::Gradient(const FVector& P)
{
	constexpr double Step = 1.5;
	return FVector(Value(P + FVector(Step, 0, 0)) - Value(P - FVector(Step, 0, 0)),
		Value(P + FVector(0, Step, 0)) - Value(P - FVector(0, Step, 0)),
		Value(P + FVector(0, 0, Step)) - Value(P - FVector(0, 0, Step))) / (2 * Step);
}

void FUghStackField::Build()
{
	Values.SetNumUninitialized(Columns * Rows * LayerCount);
	ParallelFor(LayerCount, [&](int32 K)
	{
		for (int32 J = 0; J < Rows; ++J)
		{
			for (int32 I = 0; I < Columns; ++I)
			{
				Values[(K * Rows + J) * Columns + I] = Value(Node(I, J, K));
			}
		}
	});
}

float FUghStackField::Sample(const FVector& Point) const
{
	const FVector G = (Point - Origin) / Cell;
	const double FI = FMath::Clamp(G.X, 0.0, Columns - 1.001), FJ = FMath::Clamp(G.Y, 0.0, Rows - 1.001),
		FK = FMath::Clamp(G.Z, 0.0, LayerCount - 1.001);
	const int32 I = FMath::FloorToInt32(FI), J = FMath::FloorToInt32(FJ), K = FMath::FloorToInt32(FK);
	auto Plane = [&](int32 L)
	{
		return FMath::Lerp(FMath::Lerp(At(I, J, L), At(I + 1, J, L), FI - I),
			FMath::Lerp(At(I, J + 1, L), At(I + 1, J + 1, L), FI - I), FJ - J);
	};
	return FMath::Lerp(Plane(K), Plane(K + 1), FK - K);
}

TArray<FUghDecoration> FUghStackField::Plants(const TArray<FVector>& Points) const
{
	using EKind = FUghDecoration::EKind;
	FRandomStream Random(PlantSeed);
	TArray<FUghDecoration> Plants;
	// a kind by a number 0 .. 1 (a share each), its height (pixels) and how wide for that
	struct FKind
	{
		EKind Kind;
		double Share, Lowest, Highest, Wide;
	};
	const FKind OnTop[] = { { EKind::Palm, 0.2, 120, 220, 0.8 }, { EKind::Bush, 0.35, 40, 80, 1.4 },
		{ EKind::Plant, 0.2, 25, 45, 1.1 }, { EKind::Fern, 0.2, 15, 25, 1.5 }, { EKind::Rock, 0.05, 10, 20, 1.6 } };
	const FKind OnLedges[] = { { EKind::Bush, 0.5, 20, 45, 1.4 }, { EKind::Plant, 0.25, 15, 30, 1.1 },
		{ EKind::Fern, 0.25, 10, 18, 1.5 } };
	const FKind OnWalls[] = { { EKind::Creeper, 1, 60, 160, 0.3 } };
	auto Add = [&](TConstArrayView<FKind> Kinds, const FVector& At, double Yaw)
	{
		double Pick = Random.FRand();
		const FKind* Kind = &Kinds.Last();
		for (const FKind& Each : Kinds)
		{
			if ((Pick -= Each.Share) < 0)
			{
				Kind = &Each;
				break;
			}
		}
		FUghDecoration Plant;
		Plant.Kind = Kind->Kind;
		Plant.X = At.X;
		Plant.Y = At.Y;
		Plant.Height = Random.FRandRange(Kind->Lowest, Kind->Highest);
		Plant.Width = Plant.Height * Kind->Wide;
		Plant.Depth = At.Z * UghShapes::UnitsPerPixel;
		Plant.Yaw = Yaw;
		Plant.Variant = Random.RandHelper(1000);
		Plants.Add(Plant);
	};
	// which way the surface faces (y down)
	auto Outward = [](const FVector& At) { return -Gradient(At).GetSafeNormal(); };

	// on the top: from above it down to its surface
	for (double X = MiddleX - HalfWidth; X <= MiddleX + HalfWidth; X += PlantSpacing)
	{
		for (double Depth = 0; Depth <= StoneDepth; Depth += PlantSpacing)
		{
			FVector At(X + Random.FRandRange(-0.4, 0.4) * PlantSpacing, TopY - 200,
				Depth + Random.FRandRange(-0.4, 0.4) * PlantSpacing);
			while (At.Y < TopBelow && Sample(At) <= 0)
			{
				At.Y += 4;
			}
			for (double Half = 2; Half > 0.2; Half /= 2)
			{
				At.Y += Sample(At) > 0 ? -Half : Half;
			}
			if (At.Y < TopBelow && FMath::Abs(Sample(At)) < OnSurface && -Outward(At).Y >= TopFlat)
			{
				Add(OnTop, At, Random.FRandRange(0, 360));
			}
		}
	}
	// on the ledges of its walls and hanging down their tops, away from the level
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FVector& At = Points[Index];
		if (At.Y < TopBelow || At.Y > LedgesAboveSea || FromHole(At.X, At.Y) < LedgesFromHole ||
			FMath::Abs(Sample(At)) >= OnSurface)
		{
			continue;
		}
		const FVector Out = Outward(At);
		const double BelowTop = At.Y - Shape(At).Top;
		if (Index % LedgeShare == 0 && -Out.Y >= LedgeFlat)
		{
			Add(OnLedges, At, Random.FRandRange(0, 360));
		}
		else if (Index % CreeperShare == 1 && FMath::Abs(Out.Y) < WallSteep && BelowTop > CreepersFrom &&
			BelowTop < CreepersTo)
		{
			// its leaves the way the wall faces (a yaw of 0 faces the camera: world y, minus the depth)
			Add(OnWalls, At, FMath::RadiansToDegrees(FMath::Atan2(-Out.X, -Out.Z)));
		}
	}
	return Plants;
}
