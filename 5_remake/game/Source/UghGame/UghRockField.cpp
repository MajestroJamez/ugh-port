#include "UghRockField.h"

#include "Algo/BinarySearch.h"
#include "Async/ParallelFor.h"
#include "UghRockFeatures.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/**
	 * The face in front of the slab, pixels: its edges rounded this much, at least this thick, at most FaceRelief more
	 * where its relief (Relief) stands out, and how much more it bulges in the middle of a rock at least BulgeWidth
	 * from its edges.
	 */
	constexpr double EdgeRadius = 2.5, FaceBase = 0.8, FaceRelief = 2.4, FaceBulge = 0.8, BulgeWidth = 8;
	static_assert(FaceBase + FaceRelief + FaceBulge <= FUghRockField::FaceMax);
	/** The face's layers of sandstone are about this high, pixels. */
	constexpr double StrataHeight = 9;
	/**
	 * Behind the slab: the exact outline turns into the blurred one within Blend pixels; walls and ceilings reach up to
	 * GrowMax pixels further into the cave (half of it GrowDepth deep); their roughness grows with the depth.
	 */
	constexpr double Blend = 3, GrowMax = 3, GrowDepth = 15, RoughMin = 0.5, RoughMax = 2.5, RoughPerPixel = 0.05;
	/**
	 * The cave's back wall: its mean depth, its bumps, how much deeper it is behind a dark hole of the drawing, and at
	 * least this deep (behind every decoration of UghDecorations).
	 */
	constexpr double WallDepth = 46, WallBumps = 10, HoleDepth = 22, WallMin = 30;
	/** A hole of the drawing: this dark (luminance) over this many pixels around (not the cracks between its stones). */
	constexpr double HoleDark = 0.07;
	constexpr int32 HoleBlur = 5;
	/** Beyond the screen the rock closes in by about this many pixels per pixel (more here, less there). */
	constexpr double ClosingRate = 0.6;
	/**
	 * Towards the grid's edge it closes for sure (whatever the noise): by RimClosing (more than any distance) at the edge,
	 * less and less within RimWidth pixels of it - and only ever more outwards, so no thin piece of rock floats there.
	 */
	constexpr int32 RimWidth = FUghRockOutline::MarginY;
	constexpr double RimClosing = 72;
	/** How smoothly the rock meets its back wall and its stalactites and fallen rocks, pixels. */
	constexpr double WallSmooth = 10, StampSmooth = 0.8;

	double SmoothMax(double A, double B, double K)
	{
		const double H = FMath::Max(K - FMath::Abs(A - B), 0.0) / K;
		return FMath::Max(A, B) + H * H * K / 4;
	}

	double Noise(double X, double Y, double Scale)
	{
		return FMath::PerlinNoise2D(FVector2D(X, Y) * Scale);
	}

	double Noise(double X, double Y, double Depth, double Scale)
	{
		return FMath::PerlinNoise3D(FVector(X, Y, Depth) * Scale);
	}

	/**
	 * How far the face stands out at x, y (pixels), 0 .. 1: layers of sandstone (wavy, each standing out more towards
	 * its foot, a recess under it; more marked here, less there), joints across them (nearly upright cracks), broad
	 * swellings and a grain.
	 */
	double Relief(double X, double Y)
	{
		const double Layer = Y / StrataHeight * (1 + 0.35 * Noise(X, Y, 0.008)) + 1.4 * Noise(X, Y, 0.012) +
			0.3 * Noise(X, Y, 0.06);
		const double Within = Layer - FMath::Floor(Layer);   // 0 at a layer's top, 1 at its foot
		const double Strata = FMath::SmoothStep(0.0, 0.8, Within) * (1 - FMath::SmoothStep(0.8, 1.0, Within)) *
			FMath::Clamp(0.5 + 1.3 * Noise(X, Y + 1000 * FMath::Floor(Layer), 0.03), 0.0, 1.0);
		const double Crack = FMath::Abs(FMath::PerlinNoise2D(FVector2D(X * 0.05, Y * 0.012)));
		const double Joint = FMath::SmoothStep(0.0, 0.07, Crack + 0.15 * FMath::Max(Noise(X, Y, 0.02), 0.0));
		const double Swell = 0.5 + 0.5 * (0.7 * Noise(X, Y, 0.04) + 0.3 * Noise(X, Y, 0.11));
		const double Grain = 0.5 + 0.5 * Noise(X, Y, 0.35);
		return FMath::Clamp(0.5 * Strata + 0.35 * Swell + 0.15 * Grain, 0.0, 1.0) * (0.35 + 0.65 * Joint);
	}

	/** The luminance of the drawing at each pixel of the screen, blurred over Radius pixels (the holes, not the cracks). */
	TArray<float> BlurredLuminance(TConstArrayView<FColor> Art, int32 Radius)
	{
		TArray<float> Luminance;
		Luminance.Init(1.f, Width * Height);
		if (Art.Num() != Width * Height)
		{
			return Luminance;
		}
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				float Sum = 0;
				int32 Count = 0;
				for (int32 SY = FMath::Max(Y - Radius, 0); SY <= FMath::Min(Y + Radius, Height - 1); ++SY)
				{
					for (int32 SX = FMath::Max(X - Radius, 0); SX <= FMath::Min(X + Radius, Width - 1); ++SX)
					{
						const FColor& C = Art[SY * Width + SX];
						Sum += (0.3f * C.R + 0.59f * C.G + 0.11f * C.B) / 255.f;
						++Count;
					}
				}
				Luminance[Y * Width + X] = Sum / Count;
			}
		}
		return Luminance;
	}
}

TConstArrayView<double> FUghRockField::Depths()
{
	static const TArray<double> Layers = []
	{
		// a pixel apart where the shapes are small, further apart deeper
		TArray<double> Result;
		for (double Depth = FrontDepth - 2; Depth <= 12; Depth += 1)
		{
			Result.Add(Depth);
		}
		for (double Depth = 14; Depth <= 30; Depth += 2)
		{
			Result.Add(Depth);
		}
		for (double Depth = 34; Depth <= BackDepth + 4; Depth += 4)
		{
			Result.Add(Depth);
		}
		return Result;
	}();
	return Layers;
}

void FUghRockField::Build(const ugh_logic* Logic, TConstArrayView<FColor> Art)
{
	Values.Reset();
	if (ugh_logic_pad_count(Logic) == 0)
	{
		return;   // no level yet
	}
	Outline.Build(Logic);
	MakeBackWall(Art);
	const TConstArrayView<double> Layers = Depths();
	Values.SetNumUninitialized(Columns * Rows * Layers.Num());
	ParallelFor(Layers.Num(), [&](int32 K)
	{
		const double Depth = Layers[K];
		for (int32 J = 0; J < Rows; ++J)
		{
			for (int32 I = 0; I < Columns; ++I)
			{
				Values[(K * Rows + J) * Columns + I] = Depth < -SlabHalf ? Front(I, J, Depth)
					: Depth > SlabHalf ? Behind(I, J, Depth)
					: Outline.Distance(I, J) + Closing(I, J);
			}
		}
	});
	for (const FUghRockStamp& Each : UghRockFeatures::Plan(Logic))
	{
		Stamp(Each);
	}
}

double FUghRockField::Closing(int32 I, int32 J)
{
	const double Outside = FUghRockOutline::Outside(I, J);
	if (Outside == 0)
	{
		return 0;
	}
	// and closed at the grid's rim, so that every row of the plane meets the rock
	const int32 Rim = FMath::Min(FMath::Min(I, Columns - 1 - I), FMath::Min(J, Rows - 1 - J));
	return ClosingRate * Outside * (1 + 0.6 * Noise(FUghRockOutline::X(I), FUghRockOutline::Y(J), 0.04)) +
		RimClosing * FMath::Square(FMath::Max(1 - double(Rim) / RimWidth, 0.0));
}

float FUghRockField::Front(int32 I, int32 J, double Depth) const
{
	const double X = FUghRockOutline::X(I), Y = FUghRockOutline::Y(J);
	const double Ahead = -SlabHalf - Depth;   // in front of the slab
	const double Soft = Outline.Soft(I, J);
	const double Base = FMath::Lerp(double(Outline.Distance(I, J)), Soft, FMath::SmoothStep(0.0, EdgeRadius, Ahead)) +
		Closing(I, J);
	// the edge rounded: a quarter circle from the slab's wall to the face
	const double Inset = Ahead < EdgeRadius ? EdgeRadius - FMath::Sqrt(EdgeRadius * EdgeRadius - Ahead * Ahead)
		: EdgeRadius + 3 * (Ahead - EdgeRadius);
	const double Face = FaceBase + FaceRelief * Relief(X, Y) + FaceBulge * FMath::Clamp(Soft / BulgeWidth, 0.0, 1.0);
	return -SmoothMax(Inset - Base, Ahead - Face, 1);
}

float FUghRockField::Behind(int32 I, int32 J, double Depth) const
{
	const double X = FUghRockOutline::X(I), Y = FUghRockOutline::Y(J);
	const double Behind = Depth - SlabHalf;
	const double Blended = FMath::SmoothStep(0.0, Blend, Behind);
	double Value = FMath::Lerp(double(Outline.Distance(I, J)), double(Outline.Soft(I, J)), Blended) +
		Closing(I, J);
	// walls (the outline grows sideways) and ceilings (it grows upwards: y down) reach into the cave, floors stay
	const FVector2f Way = Outline.SoftGradient(I, J).GetSafeNormal(0.01f);
	const double Wall = FMath::Abs(Way.X), Ceiling = FMath::Max(-Way.Y, 0.f);
	Value += Blended * GrowMax * (1 - FMath::Exp(-Behind / GrowDepth)) * (Wall + 0.6 * Ceiling);
	const double Rough = Blended * FMath::Min(RoughMin + RoughPerPixel * Behind, RoughMax) *
		(0.3 + 0.7 * FMath::Max(Wall, Ceiling));
	if (FMath::Abs(Value) < Rough + 0.5)   // only near the surface: further it cannot change the side
	{
		Value += Rough * (0.7 * Noise(X, Y, Depth, 0.18) + 0.3 * Noise(X, Y, Depth, 0.5));
	}
	return SmoothMax(Value, Depth - BackWall[FUghRockOutline::Index(I, J)], WallSmooth);
}

void FUghRockField::MakeBackWall(TConstArrayView<FColor> Art)
{
	const TArray<float> Luminance = BlurredLuminance(Art, HoleBlur);
	BackWall.SetNumUninitialized(Columns * Rows);
	for (int32 J = 0; J < Rows; ++J)
	{
		for (int32 I = 0; I < Columns; ++I)
		{
			const double X = FUghRockOutline::X(I), Y = FUghRockOutline::Y(J);
			double Wall = WallDepth + WallBumps * (0.7 * Noise(X, Y, 0.025) + 0.3 * Noise(X, Y, 0.09));
			if (FUghRockOutline::Outside(I, J) == 0)
			{
				const float Dark = Luminance[FMath::FloorToInt32(Y) * Width + FMath::FloorToInt32(X)];
				Wall += HoleDepth * FMath::Clamp((HoleDark - Dark) / 0.04, 0.0, 1.0);
			}
			BackWall[FUghRockOutline::Index(I, J)] = FMath::Clamp(Wall, WallMin, BackDepth);
		}
	}
}

void FUghRockField::Stamp(const FUghRockStamp& Shape)
{
	const FVector& C = Shape.Centre;
	const bool bStalactite = Shape.Kind == FUghRockStamp::EKind::Stalactite;
	const double Top = bStalactite ? C.Y - 2 : C.Y - Shape.Radius - 1;
	const double Bottom = bStalactite ? C.Y + Shape.Length + 1 : C.Y + Shape.Radius + 1;
	auto Column = [](double X) { return X + FUghRockOutline::MarginX - 0.5; };
	auto Row = [](double Y) { return Y + FUghRockOutline::MarginY - 0.5; };
	const TConstArrayView<double> Layers = Depths();
	for (int32 K = 0; K < Layers.Num(); ++K)
	{
		const double Depth = Layers[K];
		if (Depth <= SlabHalf || FMath::Abs(Depth - C.Z) > Shape.Radius + 1)
		{
			continue;   // never in the slab of the play
		}
		const int32 LastRow = FMath::Min(FMath::FloorToInt32(Row(Bottom)), Rows - 1);
		for (int32 J = FMath::Max(FMath::CeilToInt32(Row(Top)), 0); J <= LastRow; ++J)
		{
			for (int32 I = FMath::Max(FMath::CeilToInt32(Column(C.X - Shape.Radius - 1)), 0);
				I <= FMath::Min(FMath::FloorToInt32(Column(C.X + Shape.Radius + 1)), Columns - 1); ++I)
			{
				const FVector P(FUghRockOutline::X(I), FUghRockOutline::Y(J), Depth);
				double Value;
				if (bStalactite)
				{
					// thinner downwards (y down), its tip at Length
					const double Along = FMath::Max(P.Y - C.Y, 0.0) / Shape.Length;
					Value = Shape.Radius * FMath::Max(1 - Along, 0.0) - FVector2D(P.X - C.X, P.Z - C.Z).Length() -
						FMath::Max(P.Y - C.Y - Shape.Length, 0.0);
				}
				else
				{
					Value = Shape.Radius - FVector::Dist(P, C);
				}
				float& Field = Values[(K * Rows + J) * Columns + I];
				Field = SmoothMax(Field, Value, StampSmooth);
			}
		}
	}
}

float FUghRockField::Sample(const FVector& Point) const
{
	const TConstArrayView<double> Layers = Depths();
	const double FI = FMath::Clamp(Point.X + FUghRockOutline::MarginX - 0.5, 0.0, Columns - 1.001);
	const double FJ = FMath::Clamp(Point.Y + FUghRockOutline::MarginY - 0.5, 0.0, Rows - 1.001);
	const double Depth = FMath::Clamp(Point.Z, Layers[0], Layers.Last());
	const int32 K = FMath::Clamp(Algo::UpperBound(Layers, Depth) - 1, 0, Layers.Num() - 2);
	const double FK = (Depth - Layers[K]) / (Layers[K + 1] - Layers[K]);
	const int32 I = FMath::FloorToInt32(FI), J = FMath::FloorToInt32(FJ);
	const double TX = FI - I, TY = FJ - J;
	auto Plane = [&](int32 L)
	{
		return FMath::Lerp(FMath::Lerp(At(I, J, L), At(I + 1, J, L), TX),
			FMath::Lerp(At(I, J + 1, L), At(I + 1, J + 1, L), TX), TY);
	};
	return FMath::Lerp(Plane(K), Plane(K + 1), FK);
}

FVector FUghRockField::Gradient(const FVector& Point) const
{
	constexpr double Step = 0.5;
	return FVector(Sample(Point + FVector(Step, 0, 0)) - Sample(Point - FVector(Step, 0, 0)),
		Sample(Point + FVector(0, Step, 0)) - Sample(Point - FVector(0, Step, 0)),
		Sample(Point + FVector(0, 0, Step)) - Sample(Point - FVector(0, 0, Step))) / (2 * Step);
}
