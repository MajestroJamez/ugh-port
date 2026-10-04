#include "UghDecorations.h"

#include "Math/RandomStream.h"
#include "UghLedges.h"
#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	using FDecoration = FUghDecoration;

	constexpr int32 ScreenWidth = UghShapes::ScreenWidth;

	/**
	 * A palm, pixels: at least (off the pads) and at most this tall, half as wide as tall but at most PalmCanopy (so
	 * it fits between the slab of the play and the cave's back wall), with this much room left above its top; its
	 * trunk needs this much of the ledge. Only the tallest that fit are chosen from: at least PalmPreferred of the
	 * tallest.
	 */
	constexpr int32 PalmMin = 16, PalmMax = 48, PalmCanopy = 24, PalmHeadroom = 2, PalmFoot = 4;
	constexpr double PalmPreferred = 0.75;
	/** Rocks: how many, how tall (pixels; twice as wide), how many places are tried for them. */
	constexpr int32 RockCount = 2, RockMin = 4, RockMax = 8, RockTries = 40;
	/** Decoration keeps this far from the pads, from the campfire and from each other, pixels. */
	constexpr int32 OffPadMargin = 2, HearthClearance = 10, Spacing = 4;

	/** Its middle is so deep that its front (half its width deep) is behind the slab of the play. */
	double DepthFor(int32 Width)
	{
		return UghShapes::PlaneThickness / 2 + UghDecorations::SlabGap + Width * UghShapes::UnitsPerPixel / 2;
	}

	int32 Left(const FDecoration& Decoration) { return Decoration.X - Decoration.Width / 2; }

	/** The two boxes (widened by `Margin` pixels at the sides) overlap on the screen. */
	bool Overlap(const FDecoration& A, const FDecoration& B, int32 Margin)
	{
		return Left(A) - Margin < Left(B) + B.Width && Left(B) - Margin < Left(A) + A.Width &&
			A.Y - A.Height < B.Y && B.Y - B.Height < A.Y;
	}

	/**
	 * `Decoration` is too near a pad on its ledge (within `PadMargin`; PadsToo: pads do not matter), the campfire or one
	 * of `Placed`.
	 */
	bool Crowds(const ugh_logic* Logic, const FDecoration& Decoration, int32 PadMargin, const TArray<FDecoration>& Placed,
		const TOptional<FIntPoint>& Hearth)
	{
		if (PadMargin != UghLedges::PadsToo &&
			UghLedges::NearPad(Logic, Left(Decoration), Left(Decoration) + Decoration.Width, Decoration.Y, PadMargin))
		{
			return true;
		}
		if (Hearth.IsSet() && FMath::Abs(Hearth->Y - Decoration.Y) <= 1 &&
			FMath::Abs(Hearth->X - Decoration.X) < HearthClearance + Decoration.Width / 2)
		{
			return true;
		}
		return Placed.ContainsByPredicate([&](const FDecoration& Other) { return Overlap(Decoration, Other, Spacing); });
	}

	/**
	 * The tallest palm (at least `MinHeight`) that fits with its trunk at X on row Y, where `Rooms` are the empty pixels
	 * above each column of the row (its height 0 when none fits).
	 */
	FDecoration PalmAt(const TArray<int32>& Rooms, int32 X, int32 Y, int32 MinHeight)
	{
		FDecoration Palm;
		Palm.Kind = FDecoration::EKind::Palm;
		Palm.X = X;
		Palm.Y = Y;
		for (int32 Height = PalmMax; Height >= MinHeight && Palm.Height == 0; --Height)
		{
			const int32 Width = FMath::Min(PalmCanopy, Height / 2), Left = X - Width / 2;
			bool bFits = Left >= 0 && Left + Width <= ScreenWidth;
			for (int32 Column = Left; bFits && Column < Left + Width; ++Column)
			{
				bFits = Rooms[Column] >= Height + PalmHeadroom;
			}
			if (bFits)
			{
				Palm.Height = Height;
				Palm.Width = Width;
			}
		}
		return Palm;
	}

	/**
	 * The palms that fit on ledges `PadMargin` off pads (PadsToo: on pads too), at least `MinHeight` tall and at least
	 * PalmPreferred of the tallest.
	 */
	TArray<FDecoration> Palms(const ugh_logic* Logic, int32 WaterRow, const TOptional<FIntPoint>& Hearth, int32 PadMargin,
		int32 MinHeight)
	{
		TArray<FDecoration> Candidates;
		int32 Tallest = 0;
		for (const FUghLedge& Ledge : UghLedges::Find(Logic, WaterRow, MinHeight + PalmHeadroom, PadMargin))
		{
			TArray<int32> Rooms;
			for (int32 Column = 0; Column < ScreenWidth; ++Column)
			{
				Rooms.Add(UghLedges::RoomAbove(Logic, Column, Ledge.Y, PalmMax + PalmHeadroom));
			}
			for (int32 X = Ledge.First + PalmFoot / 2; X + PalmFoot / 2 <= Ledge.Last; ++X)
			{
				const FDecoration Palm = PalmAt(Rooms, X, Ledge.Y, MinHeight);
				if (Palm.Height > 0 && !Crowds(Logic, Palm, PadMargin, {}, Hearth))
				{
					Candidates.Add(Palm);
					Tallest = FMath::Max(Tallest, Palm.Height);
				}
			}
		}
		Candidates.RemoveAll([&](const FDecoration& Palm) { return Palm.Height < Tallest * PalmPreferred; });
		return Candidates;
	}

	/** A palm off the pads where one fits, else a taller one on a pad's ledge (its crown above the pad's sign). */
	void AddPalm(const ugh_logic* Logic, int32 WaterRow, const TOptional<FIntPoint>& Hearth, FRandomStream& Random,
		TArray<FDecoration>& Placed)
	{
		TArray<FDecoration> Candidates = Palms(Logic, WaterRow, Hearth, OffPadMargin, PalmMin);
		if (Candidates.IsEmpty())
		{
			Candidates = Palms(Logic, WaterRow, Hearth, UghLedges::PadsToo, UghDecorations::PalmOverPadMin);
		}
		if (!Candidates.IsEmpty())
		{
			FDecoration Palm = Candidates[Random.RandHelper(Candidates.Num())];
			Palm.Depth = DepthFor(Palm.Width);
			Palm.Yaw = Random.FRandRange(0, 360);
			Palm.Variant = Random.RandHelper(MAX_int16);
			Placed.Add(Palm);
		}
	}

	void AddRocks(const ugh_logic* Logic, int32 WaterRow, const TOptional<FIntPoint>& Hearth, FRandomStream& Random,
		TArray<FDecoration>& Placed)
	{
		// every pixel of these ledges has room for the tallest rock; a rock needs twice its height of a ledge
		TArray<FUghLedge> Ledges = UghLedges::Find(Logic, WaterRow, RockMax, OffPadMargin);
		Ledges.RemoveAll([](const FUghLedge& Ledge) { return Ledge.Length() < 2 * RockMin; });
		for (int32 Try = 0, Placing = 0; Try < RockTries && Placing < RockCount && !Ledges.IsEmpty(); ++Try)
		{
			const FUghLedge& Ledge = Ledges[Random.RandHelper(Ledges.Num())];
			FDecoration Rock;
			Rock.Height = Random.RandRange(RockMin, FMath::Min(RockMax, Ledge.Length() / 2));
			Rock.Width = 2 * Rock.Height;
			Rock.X = Random.RandRange(Ledge.First, Ledge.Last - Rock.Width) + Rock.Width / 2;   // the foot on the ledge
			Rock.Y = Ledge.Y;
			Rock.Depth = DepthFor(Rock.Width);
			Rock.Yaw = Random.FRandRange(0, 360);
			Rock.Variant = Random.RandHelper(MAX_int16);
			if (!Crowds(Logic, Rock, OffPadMargin, Placed, Hearth))
			{
				Placed.Add(Rock);
				++Placing;
			}
		}
	}
}

TArray<FUghDecoration> UghDecorations::Plan(const ugh_logic* Logic, int32 LevelId, int32 WaterRow,
	const TOptional<FIntPoint>& Hearth)
{
	FRandomStream Random(LevelId);
	TArray<FUghDecoration> Placed;
	AddPalm(Logic, WaterRow, Hearth, Random, Placed);
	AddRocks(Logic, WaterRow, Hearth, Random, Placed);
	return Placed;
}
