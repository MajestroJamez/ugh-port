#include "UghFireSim.h"

namespace
{
	/** The fuel's bed: how fast the gas leaves it (cells/s), how hot it keeps it; how fast fuel burns (1/s). */
	constexpr float FeedSpeed = 60, FeedHeat = 0.9f, BurnRate = 4;
	/** How much heat and soot a unit of fuel gives; how fast the soot thins out and the flow slows (1/s). */
	constexpr float BurnHeat = 1.1f, BurnSoot = 0.35f, SootFade = 0.6f, Drag = 0.8f;
	/** How big the stirring noise's swirls are (cells) and how fast they rise with the flame (cells/s). */
	constexpr float StirSize = 9, StirRise = 80;
	/** A finer octave of it: this much smaller, this strong. */
	constexpr float Fine = 0.45f, FineStir = 0.35f;
	/** Pressure solve: Gauss-Seidel iterations, over-relaxed. */
	constexpr int32 Iterations = 50;
	constexpr float Relax = 1.8f;
	/** How dark soot makes the light it covers. */
	constexpr float SootDark = 1.5f;

	/** The colour of glowing gas from dull red over orange and yellow to white, by its heat 0 .. 1. */
	FLinearColor Glow(float Heat)
	{
		static const FLinearColor Keys[] = { FLinearColor(0, 0, 0), FLinearColor(0.2f, 0.004f, 0.f),
			FLinearColor(1.f, 0.05f, 0.f), FLinearColor(1.f, 0.16f, 0.002f), FLinearColor(1.f, 0.36f, 0.02f),
			FLinearColor(1.f, 0.66f, 0.22f) };
		const float At = FMath::Clamp(Heat, 0.f, 1.f) * (UE_ARRAY_COUNT(Keys) - 1);
		const int32 Key = FMath::Min(int32(At), int32(UE_ARRAY_COUNT(Keys)) - 2);
		return FMath::Lerp(Keys[Key], Keys[Key + 1], At - Key);
	}
}

FUghFireSim::FUghFireSim(int32 InWidth, int32 InHeight, const FUghFireSource& InSource, int32 InSeed)
	: Width(InWidth), Height(InHeight), Source(InSource), Seed(InSeed * 17.31)
{
	U.Init(0, (Width + 1) * Height);
	V.Init(0, Width * (Height + 1));
	Heat.Init(0, Width * Height);
	Fuel.Init(0, Width * Height);
	Soot.Init(0, Width * Height);
	Pressure.Init(0, Width * Height);
}

void FUghFireSim::Advance(double Seconds)
{
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / MaxStep));
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		Step(Seconds / Steps);
	}
}

void FUghFireSim::Step(double Dt)
{
	U = Carried(U, Width + 1, Height, 0, 0.5f, Dt);
	V = Carried(V, Width, Height + 1, 0.5f, 0, Dt);
	Heat = CarriedSharp(Heat, Dt);
	Fuel = CarriedSharp(Fuel, Dt);
	Soot = Carried(Soot, Width, Height, 0.5f, 0.5f, Dt);
	Feed(Dt);
	Burn(Dt);
	Push(Dt);
	Project();
	Time += Dt;
}

void FUghFireSim::Feed(double Dt)
{
	const double Middle = Width / 2.0, Half = Source.Width * Width / 2;
	const double Bottom = Source.Lift * Width, Top = Bottom + Source.Height * Width;
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const double X = I + 0.5 - Middle, Y = J + 0.5;
			// a bed rounded at its ends (an ellipse), fed by noise along it that comes and goes
			const double Across = X / Half, Up = (Y - (Bottom + Top) / 2) / ((Top - Bottom) / 2);
			const double Inside = 1 - (Across * Across + Up * Up);
			if (Inside <= 0)
			{
				continue;
			}
			const float Gust = FMath::PerlinNoise3D(FVector(X / Half * Source.Tongues * 0.5, Time * 2.3, Seed));
			const float Feeding = float(FMath::Min(1.0, Inside * 3)) * FMath::Clamp(0.55f + 1.4f * Gust, 0.f, 1.f);
			const int32 Cell = J * Width + I;
			Fuel[Cell] = FMath::Max(Fuel[Cell], Feeding);
			Heat[Cell] = FMath::Max(Heat[Cell], FeedHeat * Feeding);
			V[Cell + Width] = FMath::Max(V[Cell + Width], FeedSpeed * Feeding);
		}
	}
}

void FUghFireSim::Burn(double Dt)
{
	const float Burnt = 1 - FMath::Exp(-BurnRate * float(Dt)), Cools = FMath::Exp(-float(Source.Cooling * Dt));
	const float Thins = FMath::Exp(-SootFade * float(Dt));
	for (int32 Cell = 0; Cell < Heat.Num(); ++Cell)
	{
		// fuel burns where it is hot enough
		const float Burning = Fuel[Cell] * Burnt * FMath::Clamp(Heat[Cell] * 4, 0.f, 1.f);
		Fuel[Cell] -= Burning;
		Heat[Cell] = (Heat[Cell] + Burning * BurnHeat) * Cools;
		Soot[Cell] = (Soot[Cell] + Burning * BurnSoot) * Thins;
	}
}

void FUghFireSim::Push(double Dt)
{
	const float Slows = FMath::Exp(-Drag * float(Dt));
	for (float& Flow : U) { Flow *= Slows; }
	for (float& Flow : V) { Flow *= Slows; }
	// the curl in the cells (from the flow in their middles), and the stirring noise's stream function at the corners
	auto Centre = [&](int32 I, int32 J)
	{
		I = FMath::Clamp(I, 0, Width - 1);
		J = FMath::Clamp(J, 0, Height - 1);
		return FVector2f(0.5f * (U[J * (Width + 1) + I] + U[J * (Width + 1) + I + 1]),
			0.5f * (V[J * Width + I] + V[(J + 1) * Width + I]));
	};
	TArray<float> Curl, Stream;
	Curl.SetNumUninitialized(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			Curl[J * Width + I] = 0.5f * (Centre(I + 1, J).Y - Centre(I - 1, J).Y - Centre(I, J + 1).X + Centre(I, J - 1).X);
		}
	}
	Stream.SetNumUninitialized((Width + 1) * (Height + 1));
	for (int32 J = 0; J <= Height; ++J)
	{
		for (int32 I = 0; I <= Width; ++I)
		{
			Stream[J * (Width + 1) + I] =
				FMath::PerlinNoise3D(FVector(I / StirSize, (J - Time * StirRise) / StirSize, Seed + 7.7)) +
				FineStir * FMath::PerlinNoise3D(
					FVector(I / (StirSize * Fine), (J - Time * StirRise) / (StirSize * Fine), Seed + 3.3));
		}
	}
	auto CurlAt = [&](int32 I, int32 J) { return FMath::Abs(Curl[FMath::Clamp(J, 0, Height - 1) * Width +
		FMath::Clamp(I, 0, Width - 1)]); };
	const float Stir = float(Source.Turbulence * StirSize);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const int32 Cell = J * Width + I;
			// vorticity confinement: towards the swirls' cores, keeping them alive
			FVector2f Towards(CurlAt(I + 1, J) - CurlAt(I - 1, J), CurlAt(I, J + 1) - CurlAt(I, J - 1));
			Towards /= Towards.Size() + 1e-5f;
			FVector2f Force = float(Source.Swirl) * Curl[Cell] * FVector2f(Towards.Y, -Towards.X);
			// the noise's curl (it stirs only what is hot) and the buoyancy of the heat, the weight of the soot
			const float* Corners = &Stream[J * (Width + 1) + I];
			const float Below = 0.5f * (Corners[0] + Corners[1]), Above = 0.5f * (Corners[Width + 1] + Corners[Width + 2]);
			const float Left = 0.5f * (Corners[0] + Corners[Width + 1]), Right = 0.5f * (Corners[1] + Corners[Width + 2]);
			Force += Stir * Heat[Cell] * FVector2f(Above - Below, Left - Right);
			Force.Y += float(Source.Buoyancy) * (Heat[Cell] - 0.3f * Soot[Cell]);
			Force *= float(Dt) * 0.5f;
			U[J * (Width + 1) + I] += Force.X;
			U[J * (Width + 1) + I + 1] += Force.X;
			V[Cell] += J > 0 ? Force.Y : 0;
			V[Cell + Width] += Force.Y;
		}
	}
}

void FUghFireSim::Project()
{
	TArray<float> Divergence;
	Divergence.SetNumUninitialized(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			Divergence[J * Width + I] = U[J * (Width + 1) + I + 1] - U[J * (Width + 1) + I] + V[(J + 1) * Width + I] -
				V[J * Width + I];
		}
	}
	// open at the sides and the top (no pressure beyond), a floor below (no flow through it)
	for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
	{
		for (int32 J = 0; J < Height; ++J)
		{
			for (int32 I = 0; I < Width; ++I)
			{
				const int32 Cell = J * Width + I;
				const float Sum = (I > 0 ? Pressure[Cell - 1] : 0) + (I < Width - 1 ? Pressure[Cell + 1] : 0) +
					(J < Height - 1 ? Pressure[Cell + Width] : 0) + (J > 0 ? Pressure[Cell - Width] : 0);
				const float Solved = (Sum - Divergence[Cell]) / (J > 0 ? 4.f : 3.f);
				Pressure[Cell] += Relax * (Solved - Pressure[Cell]);
			}
		}
	}
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I <= Width; ++I)
		{
			const float Left = I > 0 ? Pressure[J * Width + I - 1] : 0, Right = I < Width ? Pressure[J * Width + I] : 0;
			U[J * (Width + 1) + I] -= Right - Left;
		}
	}
	for (int32 J = 1; J <= Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const float Above = J < Height ? Pressure[J * Width + I] : 0;
			V[J * Width + I] -= Above - Pressure[(J - 1) * Width + I];
		}
	}
	for (int32 I = 0; I < Width; ++I)
	{
		V[I] = 0;
	}
}

float FUghFireSim::Sample(const TArray<float>& Field, int32 NX, int32 NY, float OX, float OY, float X, float Y) const
{
	const float GX = FMath::Clamp(X - OX, 0.f, float(NX - 1)), GY = FMath::Clamp(Y - OY, 0.f, float(NY - 1));
	const int32 I = FMath::Min(int32(GX), NX - 2), J = FMath::Min(int32(GY), NY - 2);
	const float FX = GX - I, FY = GY - J;
	const float* Row = &Field[J * NX + I];
	return FMath::Lerp(FMath::Lerp(Row[0], Row[1], FX), FMath::Lerp(Row[NX], Row[NX + 1], FX), FY);
}

FVector2f FUghFireSim::Velocity(float X, float Y) const
{
	return FVector2f(Sample(U, Width + 1, Height, 0, 0.5f, X, Y), Sample(V, Width, Height + 1, 0.5f, 0, X, Y));
}

TArray<float> FUghFireSim::Carried(const TArray<float>& Field, int32 NX, int32 NY, float OX, float OY, float Dt) const
{
	TArray<float> Result;
	Result.SetNumUninitialized(Field.Num());
	for (int32 J = 0; J < NY; ++J)
	{
		for (int32 I = 0; I < NX; ++I)
		{
			// back along the flow, its middle point first (second order)
			const FVector2f At(I + OX, J + OY);
			const FVector2f Half = At - 0.5f * Dt * Velocity(At.X, At.Y);
			const FVector2f From = At - Dt * Velocity(Half.X, Half.Y);
			Result[J * NX + I] = Sample(Field, NX, NY, OX, OY, From.X, From.Y);
		}
	}
	return Result;
}

TArray<float> FUghFireSim::CarriedSharp(const TArray<float>& Field, float Dt) const
{
	const TArray<float> Forth = Carried(Field, Width, Height, 0.5f, 0.5f, Dt);
	const TArray<float> Back = Carried(Forth, Width, Height, 0.5f, 0.5f, -Dt);
	TArray<float> Result;
	Result.SetNumUninitialized(Field.Num());
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const int32 Cell = J * Width + I;
			const float Corrected = Forth[Cell] + 0.5f * (Field[Cell] - Back[Cell]);
			// within the values around where it came from
			const FVector2f From = FVector2f(I + 0.5f, J + 0.5f) - Dt * Velocity(I + 0.5f, J + 0.5f);
			const int32 FI = FMath::Clamp(int32(From.X - 0.5f), 0, Width - 2);
			const int32 FJ = FMath::Clamp(int32(From.Y - 0.5f), 0, Height - 2);
			const float* Row = &Field[FJ * Width + FI];
			const float Low = FMath::Min(FMath::Min(Row[0], Row[1]), FMath::Min(Row[Width], Row[Width + 1]));
			const float High = FMath::Max(FMath::Max(Row[0], Row[1]), FMath::Max(Row[Width], Row[Width + 1]));
			Result[Cell] = FMath::Clamp(Corrected, Low, High);
		}
	}
	return Result;
}

UghFlipbook::FFrame FUghFireSim::Light() const
{
	UghFlipbook::FFrame Frame;
	Frame.Width = Width;
	Frame.Height = Height;
	Frame.Pixels.SetNumUninitialized(Width * Height);
	for (int32 J = 0; J < Height; ++J)
	{
		for (int32 I = 0; I < Width; ++I)
		{
			const int32 Cell = J * Width + I;
			const float Hot = Heat[Cell];
			FLinearColor Light = Glow(Hot) * FMath::Pow(Hot, 4.f) * FMath::Exp(-SootDark * Soot[Cell]);
			Light.A = 1;
			Frame.Pixels[(Height - 1 - J) * Width + I] = Light;
		}
	}
	return Frame;
}
