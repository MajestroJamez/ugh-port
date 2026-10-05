#include "UghFlipbook.h"

#include "UghFlames.h"

namespace
{
	using UghFlipbook::FFrame;

	/**
	 * The light this bright of all where it burns (its brightest channel, this percentile) becomes white. The crop holds
	 * where it burns on the whole (its light over all frames more than Burns of the brightest), Margin of its size
	 * around it; what flares beyond fades out towards the cells' top (Fade of their height) and sides.
	 */
	constexpr double WhitePercentile = 0.985, Margin = 0.06, Fade = 0.12;
	constexpr float Burns = 0.06f;
	/** Each pixel of a cell averages Samples x Samples points of the frame. */
	constexpr int32 Samples = 3;

	float Brightness(const FLinearColor& Color)
	{
		return FMath::Max3(Color.R, Color.G, Color.B);
	}

	/** The light of `Frame` at a point (pixels, fractions: between its pixels), black outside it. */
	FLinearColor Sample(const FFrame& Frame, double X, double Y)
	{
		const int32 Left = FMath::FloorToInt32(X - 0.5), Top = FMath::FloorToInt32(Y - 0.5);
		const float U = float(X - 0.5 - Left), V = float(Y - 0.5 - Top);
		auto At = [&](int32 Column, int32 Row)
		{
			return Column >= 0 && Row >= 0 && Column < Frame.Width && Row < Frame.Height
				? Frame.Pixels[Row * Frame.Width + Column] : FLinearColor::Transparent;
		};
		return FMath::Lerp(FMath::Lerp(At(Left, Top), At(Left + 1, Top), U),
			FMath::Lerp(At(Left, Top + 1), At(Left + 1, Top + 1), U), V);
	}

	/** The brightness that becomes white: its percentile among the pixels that have any light. */
	float White(const TArray<FFrame>& Frames)
	{
		TArray<float> Lit;
		for (const FFrame& Frame : Frames)
		{
			for (int32 Index = 0; Index < Frame.Pixels.Num(); Index += 3)
			{
				const float Light = Brightness(Frame.Pixels[Index]);
				if (Light > UE_KINDA_SMALL_NUMBER)
				{
					Lit.Add(Light);
				}
			}
		}
		if (Lit.IsEmpty())
		{
			return 0;
		}
		const int32 Nth = FMath::Min(Lit.Num() - 1, int32(Lit.Num() * WhitePercentile));
		std::nth_element(Lit.GetData(), Lit.GetData() + Nth, Lit.GetData() + Lit.Num());
		return Lit[Nth];
	}
}

FBox2D UghFlipbook::Burning(const TArray<FFrame>& Frames, float Threshold)
{
	if (Frames.IsEmpty())
	{
		return FBox2D(ForceInit);
	}
	const int32 Width = Frames[0].Width;
	TArray<float> Mean;
	Mean.Init(0, Frames[0].Pixels.Num());
	for (const FFrame& Frame : Frames)
	{
		for (int32 Pixel = 0; Pixel < Mean.Num(); ++Pixel)
		{
			Mean[Pixel] += Brightness(Frame.Pixels[Pixel]);
		}
	}
	const float Most = FMath::Max(Mean);
	FBox2D Box(ForceInit);
	for (int32 Pixel = 0; Most > 0 && Pixel < Mean.Num(); ++Pixel)
	{
		if (Mean[Pixel] > Most * Threshold)
		{
			Box += FVector2D(Pixel % Width, Pixel / Width);
			Box += FVector2D(Pixel % Width + 1, Pixel / Width + 1);
		}
	}
	return Box;
}

TArray<FColor> UghFlipbook::Make(const TArray<FFrame>& Simulated)
{
	using namespace UghFlames;
	const float Bright = Simulated.Num() >= Frames + Blend ? White(Simulated) : 0.f;
	const FBox2D Box = Bright > 0 ? Burning(Simulated, Burns) : FBox2D(ForceInit);
	if (!Box.bIsValid)
	{
		return {};
	}
	// the cell's shape around the flame: its foot at the bottom, its middle in the middle
	const FVector2D Size = Box.GetSize() * (1 + 2 * Margin);
	const double Scale = FMath::Max(Size.X / CellWidth, Size.Y / CellHeight);   // frame pixels a cell's pixel
	const double Left = Box.GetCenter().X - CellWidth * Scale / 2, Top = Box.Max.Y + Box.GetSize().Y * Margin -
		CellHeight * Scale;
	const int32 Width = Columns * CellWidth;
	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(Width * Rows * CellHeight);
	for (int32 Cell = 0; Cell < Frames; ++Cell)
	{
		// the last frames into the first: the frame after the last is the one after the end of the blend
		const double Late = Cell < Blend ? 1 - double(Cell) / Blend : 0;
		const FFrame& Now = Simulated[Cell];
		const FFrame& Later = Simulated[Cell < Blend ? Cell + Frames : Cell];
		const int32 CellLeft = Cell % Columns * CellWidth, CellTop = Cell / Columns * CellHeight;
		for (int32 Y = 0; Y < CellHeight; ++Y)
		{
			for (int32 X = 0; X < CellWidth; ++X)
			{
				FLinearColor Light = FLinearColor::Transparent;
				for (int32 Point = 0; Point < Samples * Samples; ++Point)
				{
					const double FrameX = Left + (X + (Point % Samples + 0.5) / Samples) * Scale;
					const double FrameY = Top + (Y + (Point / Samples + 0.5) / Samples) * Scale;
					Light += FMath::Lerp(Sample(Now, FrameX, FrameY), Sample(Later, FrameX, FrameY), float(Late));
				}
				Light = Light / (Samples * Samples * Bright);
				Light *= float(FMath::SmoothStep(0.0, Fade, double(Y) / CellHeight) *
					FMath::SmoothStep(0.0, Fade, FMath::Min(X + 0.5, CellWidth - X - 0.5) / CellWidth));
				Light.A = 1;
				Pixels[(CellTop + Y) * Width + CellLeft + X] = Light.GetClamped().ToFColor(true);
			}
		}
	}
	return Pixels;
}
