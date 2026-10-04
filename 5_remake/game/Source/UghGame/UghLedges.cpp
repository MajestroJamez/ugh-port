#include "UghLedges.h"

#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	bool Solid(const ugh_logic* Logic, int32 X, int32 Y) { return ugh_logic_solid(Logic, X, Y) != 0; }
}

bool UghLedges::NearPad(const ugh_logic* Logic, int32 Left, int32 Right, int32 Y, int32 Margin)
{
	for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
	{
		ugh_logic_pad Pad;
		if (ugh_logic_get_pad(Logic, Index, &Pad) && FMath::Abs(Pad.y - Y) <= 1 && Left - Margin <= Pad.right &&
			Pad.left < Right + Margin)
		{
			return true;
		}
	}
	return false;
}

int32 UghLedges::RoomAbove(const ugh_logic* Logic, int32 X, int32 Y, int32 Limit)
{
	int32 Room = 0;
	while (Room < Limit && Y - Room - 1 >= 0 && !Solid(Logic, X, Y - Room - 1))
	{
		++Room;
	}
	return Room;
}

TArray<FUghLedge> UghLedges::Find(const ugh_logic* Logic, int32 WaterRow, int32 Room, int32 PadMargin)
{
	TArray<FUghLedge> Ledges;
	for (int32 Y = Room; Y < FMath::Min(WaterRow, Height); ++Y)
	{
		auto Fits = [&](int32 X)
		{
			return Solid(Logic, X, Y) && RoomAbove(Logic, X, Y, Room) == Room &&
				(PadMargin == PadsToo || !NearPad(Logic, X, X + 1, Y, PadMargin));
		};
		for (int32 X = 0; X < Width;)
		{
			if (!Fits(X))
			{
				++X;
				continue;
			}
			const int32 First = X;
			while (X < Width && Fits(X))
			{
				++X;
			}
			Ledges.Add({ Y, First, X });
		}
	}
	return Ledges;
}
