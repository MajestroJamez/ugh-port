#include "UghPlacer.h"

#include "UghCavePortals.h"
#include "UghGround.h"
#include "ugh_logic.h"

namespace
{
	using FDecoration = FUghDecoration;
	using EKind = FUghDecoration::EKind;

	/** A wide decoration's floor may be this uneven under it (pixels); it then stands on the lowest point. */
	constexpr double Uneven = 1.2;
	/** Its floor is sampled this far out from its middle, a part of its half width (and depth). */
	constexpr double FootSpread = 0.7;
	/** A liana on the back wall finds the wall behind it at this many points along it (and one more). */
	constexpr int32 CreeperPoints = 4;
	/** Decorations keep this far apart (pixels; units in depth), from a campfire CampfireGap pixels. */
	constexpr double Gap = 0.5, DepthGap = 5, CampfireGap = 3;
	/**
	 * A palm crowds ground cover only with its trunk, this wide (pixels): grass and flowers grow at its foot. Leafy
	 * things crowd each other and ground cover only with their middles, this part of their width: leaves mingle.
	 */
	constexpr double PalmTrunk = 3, LeafyCore = 0.5;

	bool IsLeafy(const FDecoration& Decoration)
	{
		return Decoration.Kind == EKind::Fern || Decoration.Kind == EKind::Bush || Decoration.Kind == EKind::Plant ||
			Decoration.Kind == EKind::Palm || Decoration.Hangs();
	}

	bool IsGroundCover(const FDecoration& Decoration)
	{
		return Decoration.Height <= UghDecorations::GroundCover && !Decoration.Hangs() &&
			Decoration.Kind != EKind::Campfire;
	}

	/** The boxes overlap, `Margin` pixels (and `DepthMargin` units) around them too. */
	bool Overlap(const FDecoration& A, const FDecoration& B, double Margin, double DepthMargin)
	{
		return A.Left() - Margin < B.Right() && B.Left() - Margin < A.Right() && A.Top() - Margin < B.Bottom() &&
			B.Top() - Margin < A.Bottom() && A.Front() - DepthMargin < B.Back() && B.Front() - DepthMargin < A.Back();
	}

	/** `Width` narrower (and as much shallower), the same middle. */
	FDecoration Narrowed(const FDecoration& Decoration, double Width)
	{
		FDecoration Narrow = Decoration;
		Narrow.Width = FMath::Min(Width, Decoration.Width);
		return Narrow;
	}

	/**
	 * The box with which `Decoration` keeps `Other` away (none: ground cover among ground cover, lianas on the back
	 * wall among each other - a curtain).
	 */
	TOptional<FDecoration> Body(const FDecoration& Decoration, const FDecoration& Other)
	{
		const bool bCover = IsGroundCover(Decoration), bOtherCover = IsGroundCover(Other);
		const bool bCurtain = Decoration.Kind == EKind::Creeper && Other.Kind == EKind::Creeper;
		if ((bCover && bOtherCover) || bCurtain)
		{
			return {};
		}
		if (Decoration.Kind == EKind::Palm && bOtherCover)
		{
			return Narrowed(Decoration, PalmTrunk);
		}
		if (IsLeafy(Decoration) && (IsLeafy(Other) || bOtherCover))
		{
			return Narrowed(Decoration, Decoration.Width * LeafyCore);
		}
		return Decoration;
	}
}

FUghPlacer::FUghPlacer(const FUghGround& InGround, int32 InWaterRow)
	: Ground(InGround), WaterRow(InWaterRow), Pads(Landings(InGround.GetLogic()))
{
}

TArray<FUghPlacer::FLanding> FUghPlacer::Landings(const ugh_logic* Logic)
{
	TArray<FLanding> Landings;
	for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
	{
		ugh_logic_pad Pad;
		if (ugh_logic_get_pad(Logic, Index, &Pad))
		{
			for (const UghDecorations::FPadRoom& Room : { UghDecorations::PadBody, UghDecorations::PadRotor })
			{
				Landings.Add({ Pad.left - Room.Side, Pad.right + 1 + Room.Side, Pad.y - Room.Top, Pad.y - Room.Bottom,
					Room.Front });
			}
		}
	}
	return Landings;
}

double FUghPlacer::NearestFront(const FDecoration& Decoration) const
{
	double Nearest = UghDecorations::NearestFront(Decoration);
	for (const FLanding& Pad : Pads)
	{
		if (Decoration.Left() < Pad.Right && Pad.Left < Decoration.Right() && Decoration.Top() < Pad.Bottom &&
			Pad.Top < Decoration.Bottom())
		{
			Nearest = FMath::Max(Nearest, Pad.Front);
		}
	}
	return Nearest;
}

bool FUghPlacer::Settle(FDecoration& Decoration) const
{
	if (Decoration.Kind == EKind::Creeper)
	{
		// against the nearest of the wall behind it (its back half in the wall there): it hangs free where the wall is
		// further
		double Nearest = TNumericLimits<double>::Max();
		for (int32 Point = 0; Point <= CreeperPoints; ++Point)
		{
			const double Y = Decoration.Y + Decoration.Height * (0.1 + 0.9 * Point / CreeperPoints);
			const TOptional<double> Wall = Ground.Wall(Decoration.X, Y, Decoration.Depth);
			if (!Wall.IsSet())
			{
				return false;
			}
			Nearest = FMath::Min(Nearest, *Wall);
		}
		Decoration.Depth = FMath::Max(Decoration.Depth, Nearest - Decoration.Width * UghShapes::UnitsPerPixel / 4);
		return true;
	}
	if (Decoration.Hangs())
	{
		// from below where the ceiling reaches at the depth (it grows down into the cave)
		const TOptional<double> Ceiling =
			Ground.Ceiling(Decoration.X, Decoration.Y + FUghGround::CeilingReach / 2, Decoration.Depth);
		if (Ceiling.IsSet())
		{
			Decoration.Y = *Ceiling;
		}
		return Ceiling.IsSet();
	}
	// the floor under its middle and around it (not behind: its back may lean on the back wall), even enough
	const double Across = Decoration.Width / 2 * FootSpread;
	const double Deep = Decoration.Width * UghShapes::UnitsPerPixel / 2 * FootSpread;
	double Highest = TNumericLimits<double>::Max(), Lowest = TNumericLimits<double>::Lowest();
	for (const FVector2D Offset : { FVector2D(0, 0), FVector2D(-Across, 0), FVector2D(Across, 0),
		FVector2D(0, -Deep) })
	{
		const TOptional<double> Floor = Ground.Floor(Decoration.X + Offset.X, Decoration.Y, Decoration.Depth + Offset.Y);
		if (!Floor.IsSet())
		{
			return false;
		}
		Highest = FMath::Min(Highest, *Floor);
		Lowest = FMath::Max(Lowest, *Floor);
	}
	Decoration.Y = Lowest;
	return Lowest - Highest <= Uneven;
}

bool FUghPlacer::Crowds(const FDecoration& Decoration) const
{
	for (const FDecoration& Other : Placed)
	{
		if (Other.Kind == EKind::Campfire || Decoration.Kind == EKind::Campfire)
		{
			if (Overlap(Decoration, Other, CampfireGap, CampfireGap * UghShapes::UnitsPerPixel))
			{
				return true;
			}
			continue;
		}
		const TOptional<FDecoration> A = Body(Decoration, Other), B = Body(Other, Decoration);
		if (A.IsSet() && B.IsSet() && Overlap(*A, *B, Gap, DepthGap))
		{
			return true;
		}
	}
	return false;
}

bool FUghPlacer::AtEntrance(const FDecoration& Decoration) const
{
	// ground cover may grow on the floor in front of an entrance's arch, nothing else anywhere in front of its passage
	const bool bFloor = IsGroundCover(Decoration) &&
		Decoration.Back() < FUghCavePortal::Nearest * UghShapes::UnitsPerPixel;
	return !bFloor && Ground.AtEntrance(FBox2D(FVector2D(Decoration.Left(), Decoration.Top()),
		FVector2D(Decoration.Right(), Decoration.Bottom())));
}

bool FUghPlacer::TryAdd(FDecoration Decoration)
{
	if (!Settle(Decoration))
	{
		return false;
	}
	const bool bOnScreen = Decoration.Left() >= 0 && Decoration.Right() <= UghShapes::ScreenWidth &&
		Decoration.Top() >= 0 && Decoration.Bottom() < WaterRow;
	if (!bOnScreen || Decoration.Front() < NearestFront(Decoration) || AtEntrance(Decoration) || Crowds(Decoration) ||
		!Ground.Clear(Decoration))
	{
		return false;
	}
	Placed.Add(Decoration);
	return true;
}

bool FUghPlacer::TryAddBehind(FDecoration Decoration, double Extra)
{
	// a hair further: its front computed back from its depth is not nearer than allowed
	Decoration.Depth = NearestFront(Decoration) + Extra + Decoration.Width * UghShapes::UnitsPerPixel / 2 + 0.1;
	return TryAdd(Decoration);
}
