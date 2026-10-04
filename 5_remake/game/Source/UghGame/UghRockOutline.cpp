#include "UghRockOutline.h"

#include "Algo/Accumulate.h"
#include "ugh_logic.h"

namespace
{
	constexpr float Far = 1e12f;

	/**
	 * The squared distance transform of one line (Felzenszwalb and Huttenlocher): `Line` holds 0 at a feature and Far
	 * elsewhere (or the result of the other direction); afterwards each the squared distance to the nearest one.
	 */
	void Transform(TArray<float>& Line)
	{
		const int32 Count = Line.Num();
		TArray<float> Input = Line;
		TArray<int32> Parabolas;   // the vertices of the lower envelope
		TArray<float> Bounds;      // where each of them starts
		Parabolas.Reserve(Count);
		Bounds.Reserve(Count + 1);
		auto Intersection = [&](int32 Q, int32 P)
		{
			return ((Input[Q] + float(Q) * Q) - (Input[P] + float(P) * P)) / (2.f * (Q - P));
		};
		for (int32 Q = 0; Q < Count; ++Q)
		{
			if (Input[Q] >= Far)
			{
				continue;
			}
			while (!Parabolas.IsEmpty() && Intersection(Q, Parabolas.Last()) <= Bounds.Last())
			{
				Parabolas.Pop();
				Bounds.Pop();
			}
			Bounds.Add(Parabolas.IsEmpty() ? -Far : Intersection(Q, Parabolas.Last()));
			Parabolas.Add(Q);
		}
		if (Parabolas.IsEmpty())
		{
			return;   // no feature: all Far
		}
		int32 K = 0;
		for (int32 Q = 0; Q < Count; ++Q)
		{
			while (K + 1 < Parabolas.Num() && Bounds[K + 1] < Q)
			{
				++K;
			}
			const float D = float(Q - Parabolas[K]);
			Line[Q] = D * D + Input[Parabolas[K]];
		}
	}

	/** For every cell of a Width x Height grid: the squared distance to the nearest cell where `Feature` holds. */
	template <typename TFeature>
	TArray<float> SquaredDistances(int32 Width, int32 Height, TFeature Feature)
	{
		TArray<float> Result;
		Result.SetNumUninitialized(Width * Height);
		TArray<float> Line;
		for (int32 I = 0; I < Width; ++I)
		{
			Line.SetNumUninitialized(Height);
			for (int32 J = 0; J < Height; ++J)
			{
				Line[J] = Feature(I, J) ? 0.f : Far;
			}
			Transform(Line);
			for (int32 J = 0; J < Height; ++J)
			{
				Result[J * Width + I] = Line[J];
			}
		}
		for (int32 J = 0; J < Height; ++J)
		{
			Line = TArray<float>(&Result[J * Width], Width);
			Transform(Line);
			FMemory::Memcpy(&Result[J * Width], Line.GetData(), Width * sizeof(float));
		}
		return Result;
	}
}

double FUghRockOutline::Outside(int32 I, int32 J)
{
	const double DX = FMath::Max3(-X(I), X(I) - UghShapes::ScreenWidth, 0.0);
	const double DY = FMath::Max3(-Y(J), Y(J) - UghShapes::ScreenHeight, 0.0);
	return FMath::Sqrt(DX * DX + DY * DY);
}

void FUghRockOutline::Build(const ugh_logic* Logic)
{
	TArray<bool> Mask;
	Mask.SetNumUninitialized(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const int32 PixelX = FMath::Clamp(I - MarginX, 0, UghShapes::ScreenWidth - 1);
			const int32 PixelY = FMath::Clamp(J - MarginY, 0, UghShapes::ScreenHeight - 1);
			Mask[Index(I, J)] = ugh_logic_solid(Logic, PixelX, PixelY) != 0;
		}
	}
	const TArray<float> ToAir = SquaredDistances(Width, Height, [&](int32 I, int32 J) { return !Mask[Index(I, J)]; });
	const TArray<float> ToRock = SquaredDistances(Width, Height, [&](int32 I, int32 J) { return Mask[Index(I, J)]; });
	Distances.SetNumUninitialized(Width * Height);
	for (int32 Cell = 0; Cell < Width * Height; ++Cell)
	{
		// from centre to centre less half a pixel: the border of the pixel of the other kind beside it
		const float Distance = FMath::Min(FMath::Sqrt(Mask[Cell] ? ToAir[Cell] : ToRock[Cell]) - 0.5f, MaxDistance);
		Distances[Cell] = Mask[Cell] ? Distance : -Distance;
	}
	Blur();
}

void FUghRockOutline::Blur()
{
	const int32 Radius = FMath::CeilToInt32(BlurSigma * 2.5f);
	TArray<float> Weights;
	for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
	{
		Weights.Add(FMath::Exp(-0.5f * FMath::Square(Offset / BlurSigma)));
	}
	const float Sum = Algo::Accumulate(Weights, 0.f);
	auto Pass = [&](const TArray<float>& From, TArray<float>& To, int32 StepI, int32 StepJ)
	{
		To.SetNumUninitialized(Width * Height);
		for (int32 J = 0; J < Height; ++J)
		{
			for (int32 I = 0; I < Width; ++I)
			{
				float Value = 0;
				for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
				{
					const int32 SI = FMath::Clamp(I + Offset * StepI, 0, Width - 1);
					const int32 SJ = FMath::Clamp(J + Offset * StepJ, 0, Height - 1);
					Value += Weights[Offset + Radius] * From[Index(SI, SJ)];
				}
				To[Index(I, J)] = Value / Sum;
			}
		}
	};
	TArray<float> Across;
	Pass(Distances, Across, 1, 0);
	Pass(Across, Blurred, 0, 1);
	Gradients.SetNumUninitialized(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const int32 L = FMath::Max(I - 1, 0), R = FMath::Min(I + 1, Width - 1);
			const int32 U = FMath::Max(J - 1, 0), D = FMath::Min(J + 1, Height - 1);
			Gradients[Index(I, J)] = FVector2f((Blurred[Index(R, J)] - Blurred[Index(L, J)]) / (R - L),
				(Blurred[Index(I, D)] - Blurred[Index(I, U)]) / (D - U));
		}
	}
}
