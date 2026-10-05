#include "UghStreams.h"

#include "UghDecorations.h"
#include "UghGround.h"
#include "UghLedges.h"
#include "UghPadSigns.h"
#include "UghRockField.h"
#include "UghShapes.h"
#include "ugh_logic.h"

namespace
{
	constexpr int32 Height = UghShapes::ScreenHeight;

	/** Pixels: the rock is solid this far below the water's row too (no swimmer under the waterfall's foam). */
	constexpr int32 UnderWater = 3;
	/** Pixels: the bed's floor may be this far from the ledge's row; the stream at least this long (units). */
	constexpr double BedReach = 1.2, BedMin = 100;
	/**
	 * Units: the bed is followed in steps this long, up to this near the wall (the hole); pixels: it may rise this much
	 * more than SpringHeight there.
	 */
	constexpr double BedStep = 10, SpoutClear = 5, Rising = 0.5;
	/** Units: the rock behind the spring's hole is looked for this far behind the bed's end. */
	constexpr double HoleBehind = 15;
	/**
	 * The waterfall, units: in front of the face by at least Clearance, leaving the edge a little forward of it
	 * (LipOut), then out by Spread for the square root of each pixel it has fallen (it shoots out over the rock).
	 */
	constexpr double Clearance = 6, LipOut = 3, Spread = 5;
	/** Pixels: the face is looked for from this deep (the grid's front) back to here, in steps this long. */
	constexpr double FaceFrom = FUghRockField::FrontDepth - 2, FaceTo = 2, FaceStep = 0.25;
	/** Pixels: a board, a pad's landing copter keep this far from the stream's area. */
	constexpr double Apart = 2;
	/** Pixels: around the water above the ledge (FUghStream::Course): beside its banks, above the spring's hole. */
	constexpr double Bank = 1, AboveHole = 3;

	/** The front of the rock's face at x, y (units), nearest the camera; unset when the face is not there. */
	TOptional<double> FaceFront(const FUghRockField& Field, double X, double Y)
	{
		for (double Depth = FaceFrom; Depth <= FaceTo; Depth += FaceStep)
		{
			if (Field.Sample(FVector(X, Y, Depth)) > 0)
			{
				return Depth * UghShapes::UnitsPerPixel;
			}
		}
		return {};
	}

	/** Every pixel of columns Left .. Right - 1 is solid from row Top to row Bottom. */
	bool SolidBlock(const FUghGround& Ground, int32 Left, int32 Right, int32 Top, int32 Bottom)
	{
		for (int32 Y = Top; Y <= Bottom; ++Y)
		{
			for (int32 X = Left; X < Right; ++X)
			{
				if (!Ground.Solid(X, Y))
				{
					return false;
				}
			}
		}
		return true;
	}

	/** The area of `Stream` meets what may not be near it: a pad's landing copter, a board, a cave's entrance. */
	bool Crowds(const ugh_logic* Logic, const FUghRockField& Field, const TArray<FUghPadSign>& Signs, const FBox2D& Area)
	{
		const FBox2D Around = Area.ExpandBy(Apart);
		for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
		{
			ugh_logic_pad Pad;
			if (!ugh_logic_get_pad(Logic, Index, &Pad))
			{
				continue;
			}
			for (const UghDecorations::FPadRoom& Room : { UghDecorations::PadBody, UghDecorations::PadRotor })
			{
				if (Around.Intersect(FBox2D(FVector2D(Pad.left - Room.Side, Pad.y - Room.Top),
					FVector2D(Pad.right + 1 + Room.Side, Pad.y - Room.Bottom))))
				{
					return true;
				}
			}
		}
		return Signs.ContainsByPredicate([&](const FUghPadSign& Sign) { return Around.Intersect(Sign.Box()); }) ||
			Field.GetPortals().ContainsByPredicate([&](const FUghCavePortal& Portal)
			{
				return Around.Intersect(Portal.Reach());
			});
	}

	/** The stream of the ledge at row `Y` with its left bank at `Left` if it has one there. */
	TOptional<FUghStream> At(const ugh_logic* Logic, const FUghRockField& Field, const FUghGround& Ground, int32 Y,
		int32 Left, int32 WaterRow, const TArray<FUghPadSign>& Signs)
	{
		FUghStream Stream;
		Stream.Y = Y;
		Stream.Left = Left;
		Stream.Right = Left + UghStreams::Width;
		Stream.Foot = WaterRow;
		const FBox2D Area = Stream.Area();
		if (!SolidBlock(Ground, FMath::FloorToInt32(Area.Min.X), FMath::CeilToInt32(Area.Max.X), Y,
			FMath::Min(WaterRow + UnderWater, Height - 1)) || Crowds(Logic, Field, Signs, Area))
		{
			return {};
		}
		// the back wall behind the ledge where the spring's hole is looked for; the floor all the way from the bridge to
		// it (rising where it curves up into the wall: the water runs down there), the hole just above its end
		const double X = Stream.X();
		const TOptional<double> Wall = Ground.Wall(X, Y - UghStreams::SpringHeight, UghStreams::BridgeBack);
		if (!Wall || *Wall - SpoutClear < UghStreams::BridgeBack + BedMin)
		{
			return {};
		}
		Stream.Spring = *Wall;
		const double Near = UghStreams::BridgeBack - BedStep, Far = Stream.Spring - SpoutClear;
		const int32 Steps = FMath::CeilToInt32((Far - Near) / BedStep);
		auto OnBed = [&](double Floor)
		{
			return Floor >= Y - UghStreams::SpringHeight - Rising && Floor <= Y + BedReach;
		};
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const double Depth = FMath::Lerp(Near, Far, Step / double(Steps));
			const TOptional<double> LeftFloor = Ground.Floor(Stream.Left, Y, Depth);
			const TOptional<double> MiddleFloor = Ground.Floor(X, Y, Depth);
			const TOptional<double> RightFloor = Ground.Floor(Stream.Right, Y, Depth);
			if (!LeftFloor || !MiddleFloor || !RightFloor || !OnBed(*LeftFloor) || !OnBed(*MiddleFloor) ||
				!OnBed(*RightFloor))
			{
				return {};
			}
			// (raised at its banks where its middle is higher: the water lies across them)
			const double Raise = FMath::Max((*LeftFloor + *RightFloor) / 2 - *MiddleFloor, 0.0);
			Stream.Bed.Add(FVector(Depth, *LeftFloor - Raise, *RightFloor - Raise));
		}
		Stream.SpringHeight = Y - (Stream.Bed.Last().Y + Stream.Bed.Last().Z) / 2;
		// the hole: in the wall right behind the bed's end
		if (Field.Sample(FVector(X, Y - Stream.SpringHeight - 1, (Far + HoleBehind) / UghShapes::UnitsPerPixel)) <= 0)
		{
			return {};
		}
		// the waterfall in front of the face, never back towards it, shooting out a little
		double Lip = TNumericLimits<double>::Max();
		for (double Down = 0; Y + Down <= WaterRow + 1; Down += UghStreams::FallStep)
		{
			double Face = TNumericLimits<double>::Max();
			for (const double Along : { Stream.Left - 1, X, Stream.Right + 1 })
			{
				const TOptional<double> Front = FaceFront(Field, Along, Y + FMath::Max(Down, 0.5));
				if (!Front)
				{
					return {};
				}
				Face = FMath::Min(Face, *Front);
			}
			Lip = FMath::Min(Lip, Face);
			const double Free = Lip - LipOut - Spread * FMath::Sqrt(Down);
			Stream.Fall.Add(FMath::Min(FMath::Min(Free, Face - Clearance), Stream.Fall.IsEmpty() ? 0 : Stream.Fall.Last()));
		}
		Stream.BridgeLeft = Stream.Left - UghStreams::Overhang;
		Stream.BridgeRight = Stream.Right + UghStreams::Overhang;
		Stream.BridgeFront = Stream.Fall[0] - LipOut;
		Stream.BridgeBack = UghStreams::BridgeBack;
		return Stream;
	}
}

FBox2D FUghStream::Area() const
{
	return FBox2D(FVector2D(Left - UghStreams::Margin, Y - UghStreams::Room),
		FVector2D(Right + UghStreams::Margin, Foot + UnderWater + 1));
}

FBox2D FUghStream::Course() const
{
	return FBox2D(FVector2D(Left - Bank, Y - SpringHeight - AboveHole), FVector2D(Right + Bank, Y + 1));
}

TArray<FUghStream> UghStreams::Plan(const ugh_logic* Logic, const FUghRockField& Field, int32 WaterRow,
	const TArray<FUghPadSign>& Signs)
{
	if (Field.IsEmpty() || WaterRow >= Height - UnderWater)
	{
		return {};
	}
	const FUghGround Ground(Logic, Field);
	TArray<FUghStream> Streams;
	for (const FUghLedge& Ledge : UghLedges::Find(Logic, WaterRow - MinDrop + 1, Room, PadMargin))
	{
		for (int32 Left = Ledge.First + Margin; Left + Width + Margin <= Ledge.Last; ++Left)
		{
			if (TOptional<FUghStream> Stream = At(Logic, Field, Ground, Ledge.Y, Left, WaterRow, Signs))
			{
				Streams.Add(MoveTemp(*Stream));
			}
		}
	}
	// the highest waterfalls, the one nearest the middle of the screen of those
	Streams.StableSort([](const FUghStream& A, const FUghStream& B)
	{
		const double Middle = UghShapes::ScreenWidth / 2.0;
		return A.Y != B.Y ? A.Y < B.Y : FMath::Abs(A.X() - Middle) < FMath::Abs(B.X() - Middle);
	});
	TArray<FUghStream> Chosen;
	for (FUghStream& Stream : Streams)
	{
		const bool bApart = !Chosen.ContainsByPredicate([&](const FUghStream& Other)
		{
			return Other.Area().ExpandBy(Margin).Intersect(Stream.Area());
		});
		if (Chosen.Num() < MaxStreams && bApart)
		{
			Chosen.Add(MoveTemp(Stream));
		}
	}
	return Chosen;
}

TArray<FVector4> UghStreams::Feet(const TArray<FUghStream>& Streams)
{
	TArray<FVector4> Feet;
	for (const FUghStream& Stream : Streams)
	{
		const FVector At = UghShapes::ToWorld(Stream.X(), Stream.Foot, Stream.Fall.Last());
		Feet.Add(FVector4(At.X, At.Y, At.Z, (Stream.Right - Stream.Left) * UghShapes::UnitsPerPixel));
	}
	return Feet;
}

TArray<UghStreams::FRoom> UghStreams::Rooms(const FUghStream& Stream)
{
	// the bridge as high as the figures walking over it
	constexpr double Deck = UghDecorations::Middle, BehindDeck = 5;
	return {
		{ Stream.Course(), UE_BIG_NUMBER },
		{ FBox2D(FVector2D(Stream.BridgeLeft - Bank, Stream.Y - Deck), FVector2D(Stream.BridgeRight + Bank,
			Stream.Y + 1)), Stream.BridgeBack + BehindDeck } };
}

double UghStreams::Channel(const FUghStream& Stream, const FVector& Point)
{
	const double Depth = Point.Z * UghShapes::UnitsPerPixel;
	if (FMath::Abs(Point.Z) < ChannelFrom || Stream.Bed.IsEmpty())
	{
		return -1;
	}
	// the floor over it: the ledge's in front of the slab, its bed's behind it (between the bed's steps)
	double Floor = Stream.Y;
	if (Depth > 0)
	{
		const int32 Next = Stream.Bed.FindLastByPredicate([&](const FVector& Step) { return Step.X <= Depth; }) + 1;
		const FVector& Far = Stream.Bed[FMath::Clamp(Next, 0, Stream.Bed.Num() - 1)];
		const FVector& Near = Stream.Bed[FMath::Clamp(Next - 1, 0, Stream.Bed.Num() - 1)];
		const double Along = Far.X > Near.X ? FMath::Clamp((Depth - Near.X) / (Far.X - Near.X), 0.0, 1.0) : 0;
		Floor = FMath::Lerp(Near.Y + Near.Z, Far.Y + Far.Z, Along) / 2;
	}
	const double Across = Width / 2 - FMath::Abs(Point.X - Stream.X()), Under = Floor + ChannelDepth - Point.Y;
	// out of the hole in the wall: a tunnel HoleHeight high over the channel, HoleDeep into the wall
	const double End = Stream.Bed.Last().X;
	const double Roof = Depth <= End ? UE_BIG_NUMBER : Point.Y - (Floor - HoleHeight);
	const double Back = (End + HoleDeep - Depth) / UghShapes::UnitsPerPixel;
	return FMath::Min(FMath::Min(Across, Under), FMath::Min(Roof, Back));
}
