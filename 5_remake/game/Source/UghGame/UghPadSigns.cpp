#include "UghPadSigns.h"

#include "UghGround.h"
#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	/** The original's board `Tile` stands on `Pad`: its bottom on the pad's surface, the tile over the pad. */
	bool OnPad(const FUghArtTile& Tile, const ugh_logic_pad& Pad)
	{
		return Tile.At.Y + UghPadSigns::Height == Pad.y && Tile.At.X <= Pad.right &&
			Tile.At.X + UghPadSigns::Width > Pad.left;
	}

	FBox2D BoxAt(double PostX, double Ground)
	{
		const FVector2D TopLeft(PostX - UghPadSigns::PostX, Ground - UghPadSigns::Height);
		return FBox2D(TopLeft, TopLeft + FVector2D(UghPadSigns::Width, UghPadSigns::Height));
	}

	/**
	 * Where a board the original does not draw stands on `Pad`: the post on the screen over the pad as near its middle
	 * as the board is out of the caves' entrances and of the boards `Placed`; the middle when there is no such place.
	 */
	double PostOn(const ugh_logic_pad& Pad, const FUghGround& Ground, const TArray<FUghPadSign>& Placed)
	{
		const double Lowest = FMath::Max(double(Pad.left), UghPadSigns::PostX);
		const double Highest = FMath::Min(double(Pad.right), UghShapes::ScreenWidth - UghPadSigns::Width +
			UghPadSigns::PostX);
		const double Middle = FMath::Clamp((Pad.left + Pad.right) / 2.0, Lowest, Highest);
		for (double Away = 0; Middle - Away >= Lowest || Middle + Away <= Highest; Away += 1)
		{
			for (const double X : { Middle - Away, Middle + Away })
			{
				const FBox2D Box = BoxAt(X, Pad.y);
				const bool bFree = X >= Lowest && X <= Highest && !Ground.AtEntrance(Box) &&
					!Placed.ContainsByPredicate([&](const FUghPadSign& Other) { return Other.Box().Intersect(Box); });
				if (bFree)
				{
					return X;
				}
			}
		}
		return Middle;
	}
}

FBox2D FUghPadSign::Box() const
{
	return BoxAt(X, Y);
}

int32 UghPadSigns::Marks(int32 Number)
{
	return Number >= 1 && Number <= MostMarks ? Number : 0;
}

TOptional<int32> UghPadSigns::MarksOfSprite(int32 Sprite)
{
	if (Sprite < FUghLevelArt::FirstSign || Sprite > FUghLevelArt::LastSign)
	{
		return {};
	}
	return Sprite == FUghLevelArt::LastSign ? 0 : Sprite - FUghLevelArt::FirstSign + 1;
}

int32 UghPadSigns::SpriteOf(int32 Marks)
{
	return Marks >= 1 && Marks <= MostMarks ? FUghLevelArt::FirstSign + Marks - 1 : FUghLevelArt::LastSign;
}

TArray<FUghPadSign> UghPadSigns::Plan(const ugh_logic* Logic, const FUghGround& Ground, const TArray<FUghArtTile>& Drawn)
{
	TArray<ugh_logic_pad> Pads;
	TArray<FUghPadSign> Signs;
	TArray<bool> Used;
	Used.Init(false, Drawn.Num());
	for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
	{
		ugh_logic_pad& Pad = Pads.AddDefaulted_GetRef();
		ugh_logic_get_pad(Logic, Index, &Pad);
		FUghPadSign& Sign = Signs.Add_GetRef({ Index, Pad.number, Marks(Pad.number) });
		Sign.Y = Pad.y;
		for (int32 Tile = 0; Tile < Drawn.Num() && !Sign.Sprite; ++Tile)
		{
			if (!Used[Tile] && OnPad(Drawn[Tile], Pad))
			{
				Used[Tile] = true;
				Sign.Sprite = Drawn[Tile].Sprite;
				Sign.X = Drawn[Tile].At.X + PostX;
			}
		}
	}
	// the boards of their own after the drawn ones, out of their way
	TArray<FUghPadSign> Placed = Signs.FilterByPredicate([](const FUghPadSign& Sign) { return Sign.Sprite.IsSet(); });
	for (FUghPadSign& Sign : Signs)
	{
		if (!Sign.Sprite)
		{
			Sign.X = PostOn(Pads[Sign.Pad], Ground, Placed);
			Placed.Add(Sign);
		}
	}
	for (FUghPadSign& Sign : Signs)
	{
		Sign.Y = Ground.Floor(Sign.X, Pads[Sign.Pad].y, Depth).Get(Sign.Y);
	}
	return Signs;
}
