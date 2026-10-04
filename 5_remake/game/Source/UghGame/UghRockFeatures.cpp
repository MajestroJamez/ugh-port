#include "UghRockFeatures.h"

#include "UghLedges.h"
#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;
	/** The front of a feature stays this far behind the slab of the play, pixels. */
	constexpr double SlabGap = 1;
	constexpr double SlabHalf = UghShapes::PlaneThickness / 2 / UghShapes::UnitsPerPixel;

	/** A stalactite: one in about Every pixels of a flat ceiling, at least Spacing apart, a third of the room below. */
	constexpr uint32 StalactiteEvery = 7;
	constexpr int32 StalactiteSpacing = 5, StalactiteRoom = 15, StalactiteMax = 9;
	/** A fallen rock in about Every pixels of a floor (beside a wall: more), Radius between these. */
	constexpr uint32 BoulderEvery = 29;
	constexpr double BoulderMin = 1.2, BoulderMax = 2.6;

	bool Solid(const ugh_logic* Logic, int32 X, int32 Y) { return ugh_logic_solid(Logic, X, Y) != 0; }

	/** A number of the place x, y (and a salt) that looks random. */
	uint32 Hash(int32 X, int32 Y, uint32 Salt)
	{
		uint32 H = uint32(X) * 0x8da6b343u ^ uint32(Y) * 0xd8163841u ^ Salt * 0xcb1ab31fu;
		H ^= H >> 15;
		H *= 0x2c1b3c6du;
		H ^= H >> 12;
		return H;
	}

	/** A number 0 .. 1 of the place. */
	double Fraction(int32 X, int32 Y, uint32 Salt)
	{
		return (Hash(X, Y, Salt) & 0xffff) / 65535.0;
	}

	int32 RoomBelow(const ugh_logic* Logic, int32 X, int32 Y)
	{
		int32 Room = 0;
		while (Y + Room < Height && !Solid(Logic, X, Y + Room))
		{
			++Room;
		}
		return Room;
	}

	void AddStalactites(const ugh_logic* Logic, TArray<FUghRockStamp>& Stamps)
	{
		for (int32 Y = 1; Y < Height; ++Y)
		{
			int32 Last = -StalactiteSpacing;
			for (int32 X = 1; X < Width - 1; ++X)
			{
				// a flat ceiling above x, y
				const bool bCeiling = !Solid(Logic, X, Y) && Solid(Logic, X - 1, Y - 1) && Solid(Logic, X, Y - 1) &&
					Solid(Logic, X + 1, Y - 1);
				if (!bCeiling || X - Last < StalactiteSpacing || Hash(X, Y, 1) % StalactiteEvery != 0)
				{
					continue;
				}
				const int32 Room = RoomBelow(Logic, X, Y);
				if (Room < StalactiteRoom)
				{
					continue;
				}
				Last = X;
				const double Length = FMath::Min(3 + 6 * Fraction(X, Y, 2), FMath::Min(Room / 3.0, double(StalactiteMax)));
				const double Radius = 0.6 + 0.2 * Length;
				const double Depth = SlabHalf + SlabGap + Radius + 18 * Fraction(X, Y, 3);
				Stamps.Add({ FUghRockStamp::EKind::Stalactite, FVector(X + 0.5, Y - 1, Depth), Radius, Length + 1 });
			}
		}
	}

	void AddBoulders(const ugh_logic* Logic, TArray<FUghRockStamp>& Stamps)
	{
		for (int32 Y = 1; Y < Height; ++Y)
		{
			for (int32 X = 1; X < Width - 1; ++X)
			{
				if (!Solid(Logic, X, Y) || Solid(Logic, X, Y - 1) || UghLedges::NearPad(Logic, X - 3, X + 3, Y, 0))
				{
					continue;
				}
				// at the foot of a wall rising beside it, or now and then on a floor
				const bool bFoot = Solid(Logic, X - 1, Y - 1) || Solid(Logic, X + 1, Y - 1);
				if (Hash(X, Y, 4) % (bFoot ? 2 : BoulderEvery) != 0 || UghLedges::RoomAbove(Logic, X, Y, 6) < 6)
				{
					continue;
				}
				const double Radius = FMath::Lerp(BoulderMin, BoulderMax, Fraction(X, Y, 5));
				const double Depth = SlabHalf + SlabGap + Radius + 10 * Fraction(X, Y, 6);
				Stamps.Add({ FUghRockStamp::EKind::Boulder, FVector(X + 0.5, Y - Radius * 0.4, Depth), Radius, 0 });
			}
		}
	}
}

TArray<FUghRockStamp> UghRockFeatures::Plan(const ugh_logic* Logic)
{
	TArray<FUghRockStamp> Stamps;
	if (ugh_logic_pad_count(Logic) == 0)
	{
		return Stamps;   // no level yet
	}
	AddStalactites(Logic, Stamps);
	AddBoulders(Logic, Stamps);
	return Stamps;
}
