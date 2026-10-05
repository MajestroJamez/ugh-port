// The shapes of UghStoneArt: the logo's letters, the tablet, the copter, the bone.
#include "UghStoneArt.h"

namespace
{
	using FPoint = FVector2f;

	float Segment(const FPoint& P, const FPoint& A, const FPoint& B, float Radius)
	{
		const FPoint Along = B - A;
		const float T = FMath::Clamp(((P - A) | Along) / FMath::Max(Along.SizeSquared(), 1e-6f), 0.f, 1.f);
		return FPoint::Distance(P, A + Along * T) - Radius;
	}

	float Circle(const FPoint& P, const FPoint& Centre, float Radius)
	{
		return FPoint::Distance(P, Centre) - Radius;
	}

	/** A ring of `Radius` around `Centre` but for a gap of 2 x `GapHalf` radians around the angle `Gap` (y down). */
	float Arc(const FPoint& P, const FPoint& Centre, float Radius, float Gap, float GapHalf, float Thickness)
	{
		const FPoint Q = P - Centre;
		const float Off = FMath::UnwindRadians(FMath::Atan2(Q.Y, Q.X) - Gap);
		if (FMath::Abs(Off) >= GapHalf)
		{
			return FMath::Abs(Q.Size() - Radius) - Thickness;
		}
		const FPoint End1 = Centre + Radius * FPoint(FMath::Cos(Gap - GapHalf), FMath::Sin(Gap - GapHalf));
		const FPoint End2 = Centre + Radius * FPoint(FMath::Cos(Gap + GapHalf), FMath::Sin(Gap + GapHalf));
		return FMath::Min(FPoint::Distance(P, End1), FPoint::Distance(P, End2)) - Thickness;
	}

	float RoundedBox(const FPoint& P, const FPoint& Centre, const FPoint& Half, float Radius)
	{
		const FPoint Q = FPoint(FMath::Abs(P.X - Centre.X), FMath::Abs(P.Y - Centre.Y)) - Half + FPoint(Radius, Radius);
		return FPoint(FMath::Max(Q.X, 0.f), FMath::Max(Q.Y, 0.f)).Size() + FMath::Min(FMath::Max(Q.X, Q.Y), 0.f) - Radius;
	}

	/** Two shapes joined with a fillet `K` wide. */
	float SmoothUnion(float A, float B, float K)
	{
		const float H = FMath::Clamp(0.5f + 0.5f * (B - A) / K, 0.f, 1.f);
		return FMath::Lerp(B, A, H) - K * H * (1 - H);
	}

	/** A letter of the logo: its width and its distance field, both in the units of its height (y down). */
	struct FLetter
	{
		float Width;
		float (*Field)(const FPoint&);
		float Tilt;    // degrees
		float Raise;   // up, units
	};

	constexpr float Stroke = 0.135f;

	const FLetter Letters[] = {
		{ 0.8f, [](const FPoint& P) {
			return FMath::Min3(Segment(P, { Stroke, Stroke }, { Stroke, 0.6f }, Stroke),
				Segment(P, { 0.665f, Stroke }, { 0.665f, 0.6f }, Stroke),
				Arc(P, { 0.4f, 0.6f }, 0.265f, -UE_HALF_PI, UE_HALF_PI, Stroke)); }, -5, -0.03f },
		{ 1.0f, [](const FPoint& P) {
			return FMath::Min(Arc(P, { 0.5f, 0.5f }, 0.365f, -0.2f, 0.55f, Stroke),
				Segment(P, { 0.56f, 0.55f }, { 0.85f, 0.55f }, 0.11f)); }, 3, 0.02f },
		{ 0.84f, [](const FPoint& P) {
			return FMath::Min3(Segment(P, { Stroke, Stroke }, { Stroke, 1 - Stroke }, Stroke),
				Segment(P, { 0.705f, Stroke }, { 0.705f, 1 - Stroke }, Stroke),
				Segment(P, { Stroke, 0.5f }, { 0.705f, 0.5f }, 0.12f)); }, -3, -0.02f },
		{ 0.29f, [](const FPoint& P) {
			return FMath::Min(Segment(P, { 0.145f, 0.15f }, { 0.145f, 0.6f }, 0.13f), Circle(P, { 0.145f, 0.88f }, 0.12f)); },
			8, 0.03f },
	};
	constexpr float LetterGap = 0.07f;
}

TArray<FColor> UghStoneArt::Logo(int32 Width, int32 Height)
{
	float Units = -LetterGap;
	for (const FLetter& Letter : Letters)
	{
		Units += Letter.Width + LetterGap;
	}
	const float Size = FMath::Min(Width * 0.86f / Units, Height * 0.72f);   // the letters' height, pixels
	const FPoint Corner((Width - Units * Size) / 2, (Height - Size) / 2 - Size * 0.03f);
	FLook Look;
	Look.Light = FLinearColor(0.6f, 0.53f, 0.42f);
	Look.Bevel = 0.11f * Size;
	Look.Rough = 0.012f * Size;
	Look.Feature = 0.13f * Size;
	Look.Moss = 0.7f;
	Look.Rim = 2.5f;
	Look.ShadowOffset = FVector2f(0, 0.035f * Size);
	Look.ShadowSoftness = 0.07f * Size;
	Look.ShadowOpacity = 0.8f;
	return Render(Width, Height, [Size, Corner](const FPoint& P)
	{
		float Nearest = TNumericLimits<float>::Max();
		float Left = 0;
		for (const FLetter& Letter : Letters)
		{
			// in the letter's units, turned about its middle
			const FPoint Middle(Letter.Width / 2, 0.5f);
			const FPoint Local = (P - Corner) / Size - FPoint(Left, -Letter.Raise) - Middle;
			const float Angle = FMath::DegreesToRadians(-Letter.Tilt);
			const FPoint Turned(Local.X * FMath::Cos(Angle) - Local.Y * FMath::Sin(Angle),
				Local.X * FMath::Sin(Angle) + Local.Y * FMath::Cos(Angle));
			Nearest = FMath::Min(Nearest, Letter.Field(Turned + Middle) * Size);
			Left += Letter.Width + LetterGap;
		}
		return Nearest;
	}, Look);
}

TArray<FColor> UghStoneArt::Tablet(int32 Width, int32 Height)
{
	const float Margin = Height * 0.1f;
	FLook Look;
	Look.Light = FLinearColor(0.56f, 0.5f, 0.41f);
	Look.Bevel = Height * 0.08f;
	Look.Rough = Height * 0.025f;
	Look.Feature = Height * 0.22f;
	Look.Moss = 0.35f;
	Look.Cracks = 0.5f;
	Look.Rim = 2;
	Look.ShadowOffset = FVector2f(0, Height * 0.03f);
	Look.ShadowSoftness = Height * 0.05f;
	Look.Seed = 5;
	const FPoint Centre(Width / 2.f, Height / 2.f - Margin * 0.2f), Half(Width / 2.f - Margin, Height / 2.f - Margin);
	return Render(Width, Height, [Centre, Half, Height](const FPoint& P)
	{
		return RoundedBox(P, Centre, Half, Height * 0.12f);
	}, Look);
}

TArray<FColor> UghStoneArt::Copter(int32 Width, int32 Height)
{
	FLook Look;
	Look.Light = FLinearColor(0.86f, 0.76f, 0.56f);
	Look.Dark = FLinearColor(0.56f, 0.46f, 0.32f);
	Look.Bevel = Height * 0.07f;
	Look.Rough = Height * 0.004f;
	Look.Feature = Height * 0.3f;
	Look.Grain = 0.4f;
	Look.Cracks = 0.2f;
	Look.Rim = 1.2f;
	Look.ShadowOffset = FVector2f(0, Height * 0.04f);
	Look.ShadowSoftness = Height * 0.05f;
	Look.Seed = 3;
	const float Size = FMath::Min(Width / 1.4f, Height * 0.92f);
	const FPoint Corner((Width - 1.33f * Size) / 2, (Height - Size) / 2);
	return Render(Width, Height, [Size, Corner](const FPoint& Pixel)
	{
		const FPoint P = (Pixel - Corner) / Size;
		const float Cabin = FMath::Max(RoundedBox(P, { 0.72f, 0.56f }, { 0.27f, 0.22f }, 0.12f),
			-RoundedBox(P, { 0.8f, 0.5f }, { 0.12f, 0.1f }, 0.05f));
		const float Rotor = FMath::Min3(Segment(P, { 0.15f, 0.09f }, { 1.25f, 0.09f }, 0.045f),
			Segment(P, { 0.7f, 0.09f }, { 0.7f, 0.32f }, 0.05f), Circle(P, { 0.7f, 0.09f }, 0.07f));
		const float Tail = FMath::Min(Segment(P, { 0.48f, 0.52f }, { 0.08f, 0.42f }, 0.05f),
			Segment(P, { 0.08f, 0.3f }, { 0.08f, 0.54f }, 0.035f));
		const float Skid = FMath::Min3(Segment(P, { 0.45f, 0.92f }, { 1.0f, 0.92f }, 0.04f),
			Segment(P, { 0.58f, 0.76f }, { 0.55f, 0.92f }, 0.03f), Segment(P, { 0.86f, 0.76f }, { 0.9f, 0.92f }, 0.03f));
		return FMath::Min(FMath::Min(Cabin, Rotor), FMath::Min(Tail, Skid)) * Size;
	}, Look);
}

TArray<FColor> UghStoneArt::Bone(int32 Width, int32 Height)
{
	FLook Look;
	Look.Light = FLinearColor(0.88f, 0.8f, 0.62f);
	Look.Dark = FLinearColor(0.6f, 0.5f, 0.36f);
	Look.Bevel = Height * 0.16f;
	Look.Rough = Height * 0.01f;
	Look.Feature = Height * 0.7f;
	Look.Grain = 0.5f;
	Look.Cracks = 0.25f;
	Look.Rim = 1.2f;
	Look.ShadowOffset = FVector2f(0, Height * 0.05f);
	Look.ShadowSoftness = Height * 0.07f;
	Look.Seed = 4;
	const float Knob = Height * 0.25f, Apart = Height * 0.2f, Margin = Height * 0.04f, Ends = Knob + Margin;
	return Render(Width, Height, [=](const FPoint& P)
	{
		const float Mid = Height / 2.f;
		const float Knobs = FMath::Min(
			FMath::Min(Circle(P, { Ends, Mid - Apart }, Knob), Circle(P, { Ends, Mid + Apart }, Knob)),
			FMath::Min(Circle(P, { Width - Ends, Mid - Apart }, Knob), Circle(P, { Width - Ends, Mid + Apart }, Knob)));
		// the shaft a little thinner in its middle
		const float Along = FMath::Abs(P.X - Width / 2.f) / (Width / 2.f - Ends);
		const float Shaft = Segment(P, { Ends, Mid }, { Width - Ends, Mid }, Height * FMath::Lerp(0.19f, 0.24f, Along * Along));
		return SmoothUnion(Knobs, Shaft, Height * 0.12f);
	}, Look);
}

TArray<FColor> UghStoneArt::Fade(int32 Width, float Opacity)
{
	TArray<FColor> Pixels;
	for (int32 X = 0; X < Width; ++X)
	{
		const float Alpha = Opacity * FMath::Pow(1 - float(X) / FMath::Max(Width - 1, 1), 1.6f);
		Pixels.Add(FLinearColor(0, 0, 0, Alpha).ToFColor(true));
	}
	return Pixels;
}
