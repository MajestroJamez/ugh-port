#include "UghGround.h"

#include "UghDecorations.h"
#include "UghRockField.h"
#include "ugh_logic.h"

namespace
{
	/** A surface is looked for in steps of this many pixels, then refined by halving. */
	constexpr double Step = 0.25;
	constexpr int32 Halvings = 8;
	/** The part of a box at each side that is not sampled (round shapes do not fill its corners). */
	constexpr double Inset = 0.15;
	/** Pixels above where a decoration stands that may touch the ground (its foot sinks into it). */
	constexpr double Foot = 0.75;
}

bool FUghGround::Solid(int32 X, int32 Y) const
{
	return ugh_logic_solid(Logic, X, Y) != 0;
}

bool FUghGround::Rock(double X, double Y, double Depth) const
{
	return Field.Sample(FVector(X, Y, Depth / UghShapes::UnitsPerPixel)) > 0;
}

TOptional<double> FUghGround::Floor(double X, double Y, double Depth) const
{
	double Air = Y - Reach;
	if (Rock(X, Air, Depth))
	{
		return {};
	}
	for (double Below = Air + Step; Below <= Y + Reach; Air = Below, Below += Step)
	{
		if (Rock(X, Below, Depth))
		{
			for (int32 Halving = 0; Halving < Halvings; ++Halving)
			{
				const double Middle = (Air + Below) / 2;
				(Rock(X, Middle, Depth) ? Below : Air) = Middle;
			}
			return (Air + Below) / 2;
		}
	}
	return {};
}

TOptional<double> FUghGround::Ceiling(double X, double Y, double Depth) const
{
	double Air = Y;
	if (Rock(X, Air, Depth))
	{
		return {};
	}
	for (double Above = Air - Step; Above >= Y - CeilingReach; Air = Above, Above -= Step)
	{
		if (Rock(X, Above, Depth))
		{
			for (int32 Halving = 0; Halving < Halvings; ++Halving)
			{
				const double Middle = (Air + Above) / 2;
				(Rock(X, Middle, Depth) ? Above : Air) = Middle;
			}
			return (Air + Above) / 2;
		}
	}
	return {};
}

TOptional<double> FUghGround::Wall(double X, double Y, double Depth) const
{
	constexpr double DepthStep = Step * 8 * UghShapes::UnitsPerPixel;   // the back wall is far: 2 px steps
	const double Deepest = FUghRockField::BackDepth * UghShapes::UnitsPerPixel;
	double Air = Depth;
	if (Rock(X, Y, Air))
	{
		return {};
	}
	for (double Behind = Air + DepthStep; Behind <= Deepest; Air = Behind, Behind += DepthStep)
	{
		if (Rock(X, Y, Behind))
		{
			for (int32 Halving = 0; Halving < Halvings; ++Halving)
			{
				const double Middle = (Air + Behind) / 2;
				(Rock(X, Y, Middle) ? Behind : Air) = Middle;
			}
			return (Air + Behind) / 2;
		}
	}
	return {};
}

bool FUghGround::Clear(const FUghDecoration& Decoration) const
{
	const double Width = Decoration.Width, Height = Decoration.Height;
	const int32 Across = FMath::Clamp(FMath::CeilToInt32(Width / 3) + 1, 2, 6);
	const int32 Up = FMath::Clamp(FMath::CeilToInt32(Height / 3) + 1, 2, 8);
	// the ends it touches the rock with are left out: its foot (or where it hangs from)
	const double Top = Decoration.Top() + (Decoration.Hangs() ? Foot : Inset * Height);
	const double Bottom = Decoration.Bottom() - (Decoration.Hangs() ? 0 : Foot);
	const double HalfDeep = Width * UghShapes::UnitsPerPixel / 2;
	// its front half: its back may lean on the cave's back wall (the rock hides what is in it)
	for (const double Depth : { Decoration.Depth - HalfDeep * (1 - Inset), Decoration.Depth - HalfDeep / 2 })
	{
		for (int32 I = 0; I < Across; ++I)
		{
			const double X = Decoration.Left() + Width * FMath::Lerp(Inset, 1 - Inset, I / double(Across - 1));
			for (int32 J = 0; J < Up; ++J)
			{
				if (Rock(X, FMath::Lerp(Top, Bottom, J / double(Up - 1)), Depth))
				{
					return false;
				}
			}
		}
	}
	return true;
}
