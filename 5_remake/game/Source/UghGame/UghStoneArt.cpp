#include "UghStoneArt.h"

#include "Async/ParallelFor.h"

namespace
{
	float Hash(int32 X, int32 Y, uint32 Seed)
	{
		uint32 H = uint32(X) * 0x8da6b343u ^ uint32(Y) * 0xd8163841u ^ Seed * 0xcb1ab31fu;
		H ^= H >> 13;
		H *= 0x5bd1e995u;
		H ^= H >> 15;
		return float(H & 0xffffff) / float(0x1000000);
	}

	/** Smooth noise 0 .. 1 with features about a unit apart. */
	float Noise(const FVector2f& P, uint32 Seed)
	{
		const int32 X = FMath::FloorToInt(P.X), Y = FMath::FloorToInt(P.Y);
		const float U = FMath::SmoothStep(0.f, 1.f, P.X - X), V = FMath::SmoothStep(0.f, 1.f, P.Y - Y);
		return FMath::Lerp(FMath::Lerp(Hash(X, Y, Seed), Hash(X + 1, Y, Seed), U),
			FMath::Lerp(Hash(X, Y + 1, Seed), Hash(X + 1, Y + 1, Seed), U), V);
	}

	/** Four octaves of it, 0 .. 1. */
	float Fbm(const FVector2f& P, uint32 Seed)
	{
		float Sum = 0, Weight = 0.5f, Scale = 1;
		for (int32 Octave = 0; Octave < 4; ++Octave)
		{
			Sum += Weight * Noise(P * Scale, Seed + Octave);
			Weight *= 0.5f;
			Scale *= 2.03f;
		}
		return Sum / 0.9375f;
	}

	/** How near a point is to the border between two cells of random points (0 on it): the cracks. */
	float CellBorder(const FVector2f& P, uint32 Seed)
	{
		const int32 X = FMath::FloorToInt(P.X), Y = FMath::FloorToInt(P.Y);
		float First = 9, Second = 9;
		for (int32 J = -1; J <= 1; ++J)
		{
			for (int32 I = -1; I <= 1; ++I)
			{
				const FVector2f Point(X + I + Hash(X + I, Y + J, Seed), Y + J + Hash(X + I, Y + J, Seed + 1));
				const float Distance = FVector2f::Distance(P, Point);
				Second = FMath::Min(Second, FMath::Max(First, Distance));
				First = FMath::Min(First, Distance);
			}
		}
		return Second - First;
	}

	/** Where the light comes from (top left, towards the viewer; y of the picture goes down). */
	const FVector3f LightDirection = FVector3f(-0.45f, -0.75f, 0.65f).GetSafeNormal();
	const FLinearColor MossColor(0.07f, 0.12f, 0.03f);
}

TArray<FColor> UghStoneArt::Render(int32 Width, int32 Height, const FShape& Shape, const FLook& Look)
{
	const int32 Count = Width * Height;
	const float Feature = FMath::Max(Look.Feature, 1.f);
	// the distance with its ragged outline, and how high the surface is (a rounded bevel and bumps)
	TArray<float> Distance, Surface;
	Distance.SetNumUninitialized(Count);
	Surface.SetNumUninitialized(Count);
	ParallelFor(Height, [&](int32 Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FVector2f P(X + 0.5f, Y + 0.5f);
			const float D = Shape(P) + Look.Rough * (2 * Fbm(P / (Feature * 0.3f), Look.Seed + 7) - 1);
			const float Inside = FMath::Clamp(-D / FMath::Max(Look.Bevel, 0.5f), 0.f, 1.f);
			const float Bevel = FMath::Sqrt(1 - FMath::Square(1 - Inside)) * Look.Bevel * 0.7f;
			const float Bumps = Look.Grain * Feature * 0.06f * Fbm(P / (Feature * 0.5f), Look.Seed + 11);
			Distance[Y * Width + X] = D;
			Surface[Y * Width + X] = Bevel + Bumps * Inside;
		}
	});
	auto At = [&](const TArray<float>& Values, int32 X, int32 Y)
	{
		return Values[FMath::Clamp(Y, 0, Height - 1) * Width + FMath::Clamp(X, 0, Width - 1)];
	};
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(Count);
	ParallelFor(Height, [&](int32 Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FVector2f P(X + 0.5f, Y + 0.5f);
			const float D = Distance[Y * Width + X];
			const float Shadow = Look.ShadowOpacity * (1 - FMath::SmoothStep(-Look.ShadowSoftness * 0.5f,
				Look.ShadowSoftness, At(Distance, X - FMath::RoundToInt(Look.ShadowOffset.X),
				Y - FMath::RoundToInt(Look.ShadowOffset.Y))));
			const float Cover = FMath::Clamp(0.5f - D, 0.f, 1.f);
			FLinearColor Color = FLinearColor::Black;
			if (Cover > 0)
			{
				const FVector3f Normal = FVector3f(At(Surface, X - 1, Y) - At(Surface, X + 1, Y),
					At(Surface, X, Y - 1) - At(Surface, X, Y + 1), 2).GetSafeNormal();
				const float Diffuse = FMath::Max(0.f, Normal | LightDirection);
				const float Shine = FMath::Pow(FMath::Max(0.f, Normal | (LightDirection + FVector3f::ZAxisVector).GetSafeNormal()), 24);
				// the grain, speckles, cracks and moss
				const float Mottle = FMath::SmoothStep(0.2f, 0.8f, Fbm(P / Feature, Look.Seed));
				FLinearColor Albedo = FMath::Lerp(Look.Dark, Look.Light, Mottle);
				Albedo *= 1 + Look.Grain * 0.25f * (Noise(P / 2.5f, Look.Seed + 3) - 0.5f);
				// cracks: crooked borders of cells, here and there
				const FVector2f Cell = P / (Feature * 1.7f);
				const FVector2f Crooked = Cell + 0.6f * FVector2f(Fbm(Cell * 1.5f, Look.Seed + 13) - 0.5f,
					Fbm(Cell * 1.5f, Look.Seed + 17) - 0.5f);
				const float Crack = (1 - FMath::SmoothStep(0.f, 0.04f, CellBorder(Crooked, Look.Seed + 5))) *
					FMath::SmoothStep(0.42f, 0.6f, Noise(Cell * 0.8f, Look.Seed + 19));
				Albedo *= 1 - Look.Cracks * 0.6f * Crack * FMath::SmoothStep(1.f, 4.f, -D);
				const float Moss = Look.Moss * FMath::SmoothStep(0.15f, 0.6f, -Normal.Y) *
					FMath::SmoothStep(0.45f, 0.7f, Fbm(P / (Feature * 0.6f), Look.Seed + 9));
				Albedo = FMath::Lerp(Albedo, MossColor, FMath::Clamp(Moss, 0.f, 0.85f));
				Color = Albedo * (0.34f + 0.9f * Diffuse) + FLinearColor::White * (0.12f * Shine * (1 - Moss));
				Color *= FMath::Lerp(0.3f, 1.f, FMath::SmoothStep(0.f, Look.Rim, -D));   // the rim
			}
			// over its shadow
			const float Alpha = Cover + Shadow * (1 - Cover);
			Color = Alpha > 0 ? Color * (Cover / Alpha) : FLinearColor::Black;
			Color.A = Alpha;
			Pixels[Y * Width + X] = Color.ToFColor(true);
		}
	});
	return Pixels;
}
