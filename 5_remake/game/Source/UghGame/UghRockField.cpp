#include "UghRockField.h"

#include "Algo/BinarySearch.h"
#include "Async/ParallelFor.h"
#include "UghCavePortals.h"
#include "UghRockFeatures.h"
#include "UghRockNoise.h"
#include "UghStreams.h"
#include "ugh_logic.h"

namespace
{
	using UghRockNoise::SmoothMax;

	constexpr int32 Width = UghShapes::ScreenWidth, Height = UghShapes::ScreenHeight;

	/**
	 * The face in front of the slab, pixels: at least this thick (in front of the slab's front, which shows where the
	 * face broke off: FUghRockMesh::Shade), at most FaceRelief more where its relief (Face) stands out, and up to
	 * FaceBulge more in broad swells across the whole level (one boulder's surface, not a pillow on each rock; SwellScale
	 * per pixel), from BulgeWidth pixels inside the rock's edges.
	 */
	constexpr double FaceBase = 1.4, FaceRelief = 2.8, FaceBulge = 0.8, BulgeWidth = 3, SwellScale = 0.03;
	static_assert(FaceBase + FaceRelief + FaceBulge <= FUghRockField::FaceMax);
	/**
	 * The face's slate (step 30a: no tiles' blocks of cobbles): beds about FaceBeds thick (pixels), broken into flakes
	 * about FaceFlakes long, out most at their feet (lips); through them the stone's great beds (StoneBeds, StoneFlakes,
	 * as FUghStackField's face at StoneAlong: one boulder) and thin laminae; blocks about BlockWidth x BlockHeight, a
	 * share of them (BrokenShare) broken out of the face (a hollow, BrokenDepth of the relief left).
	 */
	constexpr double FaceBeds = 7, FaceFlakes = 30, StoneBeds = 48, StoneFlakes = 110, StoneAlong = -10,
		LaminaHeight = 2.2, LaminaLength = 12, BlockWidth = 26, BlockHeight = 12, BrokenShare = 0.16, BrokenDepth = 0.3;
	/**
	 * The face's edge broken flake by flake: a flake ends up to EdgeBreak pixels inside the mask's edge (the flakes
	 * below BreakFrom reach it), a block broken out at the edge BrokenBreak; on top edges (the pads) TopKeep less. Its
	 * edge rounded EdgeSharp (a sharp lip) to EdgeWorn (weathered) pixels.
	 */
	constexpr double EdgeBreak = 7, BreakFrom = 0.3, BrokenBreak = 6, TopKeep = 0.75, EdgeSharp = 0.8, EdgeWorn = 2.5;
	/** The walls' ribs and the back wall's beds: their flakes about this long. */
	constexpr double FlakeLength = 16;
	/**
	 * The cave's walls: the slate's beds as ribs, this thick and standing out up to this much (pixels); the back wall's
	 * beds this thick and deep.
	 */
	constexpr double WallBeds = 6, WallRibs = 1.2, BackBeds = 9, BackSteps = 2;
	/**
	 * Behind the slab: the exact outline turns into the blurred one within Blend pixels; walls and ceilings reach up to
	 * GrowMax pixels further into the cave (half of it GrowDepth deep); their roughness grows with the depth.
	 */
	constexpr double Blend = 3, GrowMax = 3, GrowDepth = 15, RoughMin = 0.5, RoughMax = 2.5, RoughPerPixel = 0.05;
	/**
	 * Behind the figures' room (from LedgeFrom, fully from LedgeTo pixels behind the slab: the boards of the pads'
	 * numbers, the enemies and the copters' bodies are nearer) the slate's beds of the walls and ceilings reach into the
	 * cave as ledges: beds LedgeBeds thick, flakes LedgeFlakes long, each up to LedgeReach pixels its own way, most at
	 * its foot (a lip). Seen from the play the cave's outline is broken strata, not the tiles' rectangles. Further back
	 * (from FadeFrom to FadeTo) they sink back into the walls, and they stay out of the lowest FloorFrom .. FloorTo
	 * pixels over a floor: room for the palms and huts.
	 */
	constexpr double LedgeFrom = 9, LedgeTo = 13, FadeFrom = 15, FadeTo = 18, LedgeReach = 12, LedgeBeds = 7,
		LedgeFlakes = 26, RoomShare = 0.2;
	constexpr int32 FloorFrom = 12, FloorTo = 24, FloorAround = 12, FloorStep = 3, RoomMost = 60;
	/**
	 * The cave's back wall: its mean depth, its bumps, how much deeper it is behind a dark hole of the drawing, and at
	 * least this deep (behind every decoration of UghDecorations).
	 */
	constexpr double WallDepth = 46, WallBumps = 10, HoleDepth = 22, WallMin = 30;
	/** A hole of the drawing: this dark (luminance) over this many pixels around (not the cracks between its stones). */
	constexpr double HoleDark = 0.07;
	constexpr int32 HoleBlur = 5;
	/** Beyond the screen the rock closes in by about this many pixels per pixel (more here, less there). */
	constexpr double ClosingRate = 0.6;
	/**
	 * Towards the grid's edge it closes for sure (whatever the noise): by RimClosing (more than any distance) at the edge,
	 * less and less within RimWidth pixels of it - and only ever more outwards, so no thin piece of rock floats there.
	 */
	constexpr int32 RimWidth = FUghRockOutline::MarginY;
	constexpr double RimClosing = 72;
	/** How smoothly the rock meets its back wall and its stalactites and fallen rocks, pixels. */
	constexpr double WallSmooth = 10, StampSmooth = 0.8;
	/** How smoothly a stream's channel is cut into the rock, pixels. */
	constexpr double ChannelSmooth = 0.5;

	double Noise(double X, double Y, double Scale)
	{
		return FMath::PerlinNoise2D(FVector2D(X, Y) * Scale);
	}

	double Noise(double X, double Y, double Depth, double Scale)
	{
		return FMath::PerlinNoise3D(FVector(X, Y, Depth) * Scale);
	}

	/** The face at x, y: how far it stands out (0 .. 1), how far inside the mask's edge it ends, its edge's radius. */
	struct FFace
	{
		double Relief, Break, Radius;
	};

	/**
	 * The face at x, y (pixels), `Top` 1 on a top edge of the mask: beds of slate (flakes each standing out its own way,
	 * most at its foot - a sharp lip over the bed below -, open seams between some of them), the stone's great beds and
	 * thin laminae through them, blocks (some broken out), a grain; its edge broken flake by flake.
	 */
	FFace Face(double X, double Y, double Top)
	{
		using UghRockNoise::FBlock;
		const FBlock Bed = UghRockNoise::Slate(X, Y, X, FaceBeds, FaceFlakes, 0);
		const FBlock Stone = UghRockNoise::Slate(X, Y, X + StoneAlong, StoneBeds, StoneFlakes, 13);
		const FBlock Lamina = UghRockNoise::Slate(X, Y, X, LaminaHeight, LaminaLength, 3);
		const FBlock Block = UghRockNoise::Blocks(X, Y, BlockWidth, BlockHeight, 7);
		const bool bBroken = Block.Random < BrokenShare;
		const double Grain = 0.5 + 0.5 * Noise(X, Y, 0.35);
		FFace Face;
		Face.Relief = FMath::Clamp(0.56 * Bed.Height + 0.24 * Stone.Height + 0.1 * Lamina.Height +
			0.06 * Block.Height + 0.04 * Grain, 0.0, 1.0) * (0.3 + 0.7 * Bed.Crack) * (0.7 + 0.3 * Block.Crack) *
			(bBroken ? BrokenDepth : 1);
		Face.Break = (EdgeBreak * FMath::SmoothStep(BreakFrom, 1.0, Bed.Random) + (bBroken ? BrokenBreak : 0)) *
			(1 - TopKeep * Top);
		Face.Radius = FMath::Lerp(EdgeSharp, EdgeWorn, Bed.Random);
		return Face;
	}

	/** The luminance of the drawing at each pixel of the screen, blurred over Radius pixels (the holes, not the cracks). */
	TArray<float> BlurredLuminance(TConstArrayView<FColor> Art, int32 Radius)
	{
		TArray<float> Luminance;
		Luminance.Init(1.f, Width * Height);
		if (Art.Num() != Width * Height)
		{
			return Luminance;
		}
		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				float Sum = 0;
				int32 Count = 0;
				for (int32 SY = FMath::Max(Y - Radius, 0); SY <= FMath::Min(Y + Radius, Height - 1); ++SY)
				{
					for (int32 SX = FMath::Max(X - Radius, 0); SX <= FMath::Min(X + Radius, Width - 1); ++SX)
					{
						const FColor& C = Art[SY * Width + SX];
						Sum += (0.3f * C.R + 0.59f * C.G + 0.11f * C.B) / 255.f;
						++Count;
					}
				}
				Luminance[Y * Width + X] = Sum / Count;
			}
		}
		return Luminance;
	}
}

TConstArrayView<double> FUghRockField::Depths()
{
	static const TArray<double> Layers = []
	{
		// a pixel apart where the shapes are small, further apart deeper
		TArray<double> Result;
		for (double Depth = FrontDepth - 2; Depth <= 12; Depth += 1)
		{
			Result.Add(Depth);
		}
		for (double Depth = 14; Depth <= 30; Depth += 2)
		{
			Result.Add(Depth);
		}
		for (double Depth = 34; Depth <= BackDepth + 4; Depth += 4)
		{
			Result.Add(Depth);
		}
		return Result;
	}();
	return Layers;
}

void FUghRockField::Build(const ugh_logic* Logic, TConstArrayView<FColor> Art, TConstArrayView<FUghArtTile> Doors)
{
	Values.Reset();
	Portals.Reset();
	if (ugh_logic_pad_count(Logic) == 0)
	{
		return;   // no level yet
	}
	Outline.Build(Logic);
	MakeBackWall(Art);
	const TConstArrayView<double> Layers = Depths();
	Values.SetNumUninitialized(Columns * Rows * Layers.Num());
	ParallelFor(Layers.Num(), [&](int32 K)
	{
		const double Depth = Layers[K];
		for (int32 J = 0; J < Rows; ++J)
		{
			for (int32 I = 0; I < Columns; ++I)
			{
				Values[(K * Rows + J) * Columns + I] = Depth < -SlabHalf ? Front(I, J, Depth)
					: Depth > SlabHalf ? Behind(I, J, Depth)
					: Outline.Distance(I, J) + Closing(I, J);
			}
		}
	});
	Portals = UghCavePortals::Plan(Outline, Doors);
	for (const FUghRockStamp& Each : UghRockFeatures::Plan(Logic))
	{
		// none in front of a cave's entrance
		const FBox2D Around(FVector2D(Each.Centre.X - Each.Radius, Each.Centre.Y - Each.Radius),
			FVector2D(Each.Centre.X + Each.Radius, Each.Centre.Y + Each.Radius + Each.Length));
		if (!Portals.ContainsByPredicate([&](const FUghCavePortal& Portal)
			{
				return Portal.Reach().Intersect(Around);
			}))
		{
			Stamp(Each);
		}
	}
	for (const FUghCavePortal& Portal : Portals)
	{
		Carve(Portal);
	}
}

void FUghRockField::Carve(const FUghCavePortal& Portal)
{
	const FBox2D Reach = Portal.Reach();
	const int32 FirstColumn = FMath::Max(FMath::FloorToInt32(Reach.Min.X) + FUghRockOutline::MarginX, 0);
	const int32 LastColumn = FMath::Min(FMath::CeilToInt32(Reach.Max.X) + FUghRockOutline::MarginX, Columns - 1);
	const int32 FirstRow = FMath::Max(FMath::FloorToInt32(Reach.Min.Y) + FUghRockOutline::MarginY, 0);
	const int32 LastRow = FMath::Min(FMath::CeilToInt32(Reach.Max.Y) + FUghRockOutline::MarginY, Rows - 1);
	const TConstArrayView<double> Layers = Depths();
	ParallelFor(Layers.Num(), [&](int32 K)
	{
		if (Layers[K] < FUghCavePortal::Nearest)
		{
			return;   // never near the slab of the play
		}
		for (int32 J = FirstRow; J <= LastRow; ++J)
		{
			for (int32 I = FirstColumn; I <= LastColumn; ++I)
			{
				float& Field = Values[(K * Rows + J) * Columns + I];
				Field = Portal.Shape(FVector(FUghRockOutline::X(I), FUghRockOutline::Y(J), Layers[K]), Field);
			}
		}
	});
}

double FUghRockField::Closing(int32 I, int32 J)
{
	const double Outside = FUghRockOutline::Outside(I, J);
	if (Outside == 0)
	{
		return 0;
	}
	// and closed at the grid's rim, so that every row of the plane meets the rock
	const int32 Rim = FMath::Min(FMath::Min(I, Columns - 1 - I), FMath::Min(J, Rows - 1 - J));
	return ClosingRate * Outside * (1 + 0.6 * Noise(FUghRockOutline::X(I), FUghRockOutline::Y(J), 0.04)) +
		RimClosing * FMath::Square(FMath::Max(1 - double(Rim) / RimWidth, 0.0));
}

float FUghRockField::Front(int32 I, int32 J, double Depth) const
{
	const double X = FUghRockOutline::X(I), Y = FUghRockOutline::Y(J);
	const double Ahead = -SlabHalf - Depth;   // in front of the slab
	const double Soft = Outline.Soft(I, J);
	const double Exact = Outline.Distance(I, J) + Closing(I, J);
	// a top edge: the rock below, the air above (y down)
	const double Top = FMath::Max(Outline.SoftGradient(I, J).GetSafeNormal(0.01f).Y, 0.f);
	const FFace Shape = Face(X, Y, Top);
	const double Radius = Shape.Radius;
	const double Base = FMath::Lerp(double(Outline.Distance(I, J)), Soft, FMath::SmoothStep(0.0, Radius, Ahead)) +
		Closing(I, J) - Shape.Break;
	// the edge rounded: a quarter circle from the slab's wall (or its front, where the flake broke off) to the face
	const double Inset = Ahead < Radius ? Radius - FMath::Sqrt(Radius * Radius - Ahead * Ahead)
		: Radius + 3 * (Ahead - Radius);
	const double Swell = FMath::Clamp(0.5 + 0.7 * Noise(X, Y, SwellScale), 0.0, 1.0);
	const double Thick = FaceBase + FaceRelief * Shape.Relief +
		FaceBulge * Swell * FMath::Clamp(Soft / BulgeWidth, 0.0, 1.0);
	// never over the air of the plane of the play
	return FMath::Min(-SmoothMax(Inset - Base, Ahead - Thick, 1), Exact);
}

float FUghRockField::Behind(int32 I, int32 J, double Depth) const
{
	const double X = FUghRockOutline::X(I), Y = FUghRockOutline::Y(J);
	const double Behind = Depth - SlabHalf;
	const double Blended = FMath::SmoothStep(0.0, Blend, Behind);
	double Value = FMath::Lerp(double(Outline.Distance(I, J)), double(Outline.Soft(I, J)), Blended) +
		Closing(I, J);
	// walls (the outline grows sideways) and ceilings (it grows upwards: y down) reach into the cave, floors stay
	const FVector2f Way = Outline.SoftGradient(I, J).GetSafeNormal(0.01f);
	const double Wall = FMath::Abs(Way.X), Ceiling = FMath::Max(-Way.Y, 0.f);
	Value += Blended * GrowMax * (1 - FMath::Exp(-Behind / GrowDepth)) * (Wall + 0.6 * Ceiling);
	// behind the figures' room their beds as ledges (only near the surface: further they cannot change the side)
	const double Ledges = FMath::SmoothStep(LedgeFrom, LedgeTo, Behind) *
		(1 - FMath::SmoothStep(FadeFrom, FadeTo, Behind)) * FMath::Min(Wall + Ceiling, 1.0);
	if (Ledges > 0.01 && Value > -LedgeReach - 1 && Value < 1)
	{
		// none low over a floor (the palms' and huts' room), and the cave's middle open (at most RoomShare of the air
		// across it)
		auto Run = [&](int32 DI, int32 DJ, int32 Most)
		{
			int32 Length = 0;
			for (int32 CI = I + DI, CJ = J + DJ; Length < Most && CI >= 0 && CI < Columns && CJ < Rows &&
				!Outline.Solid(CI, CJ); CI += DI, CJ += DJ)
			{
				++Length;
			}
			return Length;
		};
		const int32 Across = 1 + Run(-1, 0, RoomMost) + Run(1, 0, RoomMost);
		// (the floor under it or beside it, within a ledge's reach: none grows round a corner up over a floor)
		int32 Room = FloorTo;
		for (int32 DI = -FloorAround; DI <= FloorAround; DI += FloorStep)
		{
			for (int32 Down = 1; Down <= Room && J + Down < Rows; ++Down)
			{
				if (Outline.Solid(FMath::Clamp(I + DI, 0, Columns - 1), J + Down))
				{
					Room = Down - 1;
				}
			}
		}
		const UghRockNoise::FBlock Ledge = UghRockNoise::Slate(X, Y, X + Depth, LedgeBeds, LedgeFlakes, 41);
		Value += Ledges * FMath::SmoothStep(double(FloorFrom), double(FloorTo), double(Room)) *
			FMath::Min(LedgeReach, RoomShare * Across) * Ledge.Height * (0.25 + 0.75 * Ledge.Random) *
			(0.4 + 0.6 * Ledge.Crack);
	}
	const double Rough = Blended * FMath::Min(RoughMin + RoughPerPixel * Behind, RoughMax) *
		(0.3 + 0.7 * FMath::Max(Wall, Ceiling));
	// (only on steep walls, not at the slab nor on floors: a stream's banks stay level)
	const double Ribs = FMath::SmoothStep(Blend, 2 * Blend, Behind) * WallRibs * FMath::SmoothStep(0.75, 0.95, Wall);
	if (FMath::Abs(Value) < Rough + Ribs + 0.5)   // only near the surface: further it cannot change the side
	{
		Value += Rough * (0.7 * Noise(X, Y, Depth, 0.18) + 0.3 * Noise(X, Y, Depth, 0.5));
		// the slate's beds along the walls (into the depth) as ribs
		if (Ribs > 0.01)
		{
			Value += Ribs * UghRockNoise::Slate(X, Y, Depth, WallBeds, FlakeLength, 21).Height;
		}
	}
	return SmoothMax(Value, Depth - BackWall[FUghRockOutline::Index(I, J)], WallSmooth);
}

void FUghRockField::MakeBackWall(TConstArrayView<FColor> Art)
{
	const TArray<float> Luminance = BlurredLuminance(Art, HoleBlur);
	BackWall.SetNumUninitialized(Columns * Rows);
	for (int32 J = 0; J < Rows; ++J)
	{
		for (int32 I = 0; I < Columns; ++I)
		{
			const double X = FUghRockOutline::X(I), Y = FUghRockOutline::Y(J);
			// bumpy, the slate's beds stepping out of it
			double Wall = WallDepth + WallBumps * (0.7 * Noise(X, Y, 0.025) + 0.3 * Noise(X, Y, 0.09)) -
				BackSteps * (UghRockNoise::Slate(X, Y, X, BackBeds, 2 * FlakeLength, 31).Height - 0.5);
			if (FUghRockOutline::Outside(I, J) == 0)
			{
				const float Dark = Luminance[FMath::FloorToInt32(Y) * Width + FMath::FloorToInt32(X)];
				Wall += HoleDepth * FMath::Clamp((HoleDark - Dark) / 0.04, 0.0, 1.0);
			}
			BackWall[FUghRockOutline::Index(I, J)] = FMath::Clamp(Wall, WallMin, BackDepth);
		}
	}
}

void FUghRockField::Stamp(const FUghRockStamp& Shape)
{
	const FVector& C = Shape.Centre;
	const bool bStalactite = Shape.Kind == FUghRockStamp::EKind::Stalactite;
	const double Top = bStalactite ? C.Y - 2 : C.Y - Shape.Radius - 1;
	const double Bottom = bStalactite ? C.Y + Shape.Length + 1 : C.Y + Shape.Radius + 1;
	auto Column = [](double X) { return X + FUghRockOutline::MarginX - 0.5; };
	auto Row = [](double Y) { return Y + FUghRockOutline::MarginY - 0.5; };
	const TConstArrayView<double> Layers = Depths();
	for (int32 K = 0; K < Layers.Num(); ++K)
	{
		const double Depth = Layers[K];
		if (Depth <= SlabHalf || FMath::Abs(Depth - C.Z) > Shape.Radius + 1)
		{
			continue;   // never in the slab of the play
		}
		const int32 LastRow = FMath::Min(FMath::FloorToInt32(Row(Bottom)), Rows - 1);
		for (int32 J = FMath::Max(FMath::CeilToInt32(Row(Top)), 0); J <= LastRow; ++J)
		{
			for (int32 I = FMath::Max(FMath::CeilToInt32(Column(C.X - Shape.Radius - 1)), 0);
				I <= FMath::Min(FMath::FloorToInt32(Column(C.X + Shape.Radius + 1)), Columns - 1); ++I)
			{
				const FVector P(FUghRockOutline::X(I), FUghRockOutline::Y(J), Depth);
				double Value;
				if (bStalactite)
				{
					// thinner downwards (y down), its tip at Length
					const double Along = FMath::Max(P.Y - C.Y, 0.0) / Shape.Length;
					Value = Shape.Radius * FMath::Max(1 - Along, 0.0) - FVector2D(P.X - C.X, P.Z - C.Z).Length() -
						FMath::Max(P.Y - C.Y - Shape.Length, 0.0);
				}
				else
				{
					Value = Shape.Radius - FVector::Dist(P, C);
				}
				float& Field = Values[(K * Rows + J) * Columns + I];
				Field = SmoothMax(Field, Value, StampSmooth);
			}
		}
	}
}

float FUghRockField::Sample(const FVector& Point) const
{
	const TConstArrayView<double> Layers = Depths();
	const double FI = FMath::Clamp(Point.X + FUghRockOutline::MarginX - 0.5, 0.0, Columns - 1.001);
	const double FJ = FMath::Clamp(Point.Y + FUghRockOutline::MarginY - 0.5, 0.0, Rows - 1.001);
	const double Depth = FMath::Clamp(Point.Z, Layers[0], Layers.Last());
	const int32 K = FMath::Clamp(Algo::UpperBound(Layers, Depth) - 1, 0, Layers.Num() - 2);
	const double FK = (Depth - Layers[K]) / (Layers[K + 1] - Layers[K]);
	const int32 I = FMath::FloorToInt32(FI), J = FMath::FloorToInt32(FJ);
	const double TX = FI - I, TY = FJ - J;
	auto Plane = [&](int32 L)
	{
		return FMath::Lerp(FMath::Lerp(At(I, J, L), At(I + 1, J, L), TX),
			FMath::Lerp(At(I, J + 1, L), At(I + 1, J + 1, L), TX), TY);
	};
	return FMath::Lerp(Plane(K), Plane(K + 1), FK);
}

FVector FUghRockField::Gradient(const FVector& Point) const
{
	constexpr double Step = 0.5;
	return FVector(Sample(Point + FVector(Step, 0, 0)) - Sample(Point - FVector(Step, 0, 0)),
		Sample(Point + FVector(0, Step, 0)) - Sample(Point - FVector(0, Step, 0)),
		Sample(Point + FVector(0, 0, Step)) - Sample(Point - FVector(0, 0, Step))) / (2 * Step);
}

void FUghRockField::CarveChannels(TConstArrayView<FUghStream> Streams)
{
	const TConstArrayView<double> Layers = Depths();
	for (const FUghStream& Stream : Streams)
	{
		const int32 FirstColumn = FMath::Max(FMath::FloorToInt32(Stream.Left) - 1 + FUghRockOutline::MarginX, 0);
		const int32 LastColumn = FMath::Min(FMath::CeilToInt32(Stream.Right) + 1 + FUghRockOutline::MarginX, Columns - 1);
		const int32 FirstRow = FMath::Max(FMath::FloorToInt32(Stream.Y - Stream.SpringHeight - UghStreams::HoleHeight) -
			1 + FUghRockOutline::MarginY, 0);
		const int32 LastRow = FMath::Min(FMath::CeilToInt32(Stream.Y + UghStreams::ChannelDepth) + 2 +
			FUghRockOutline::MarginY, Rows - 1);
		ParallelFor(Layers.Num(), [&](int32 K)
		{
			if (FMath::Abs(Layers[K]) < UghStreams::ChannelFrom)
			{
				return;   // never in the slab of the play
			}
			for (int32 J = FirstRow; J <= LastRow; ++J)
			{
				for (int32 I = FirstColumn; I <= LastColumn; ++I)
				{
					float& Field = Values[(K * Rows + J) * Columns + I];
					Field = -SmoothMax(-Field, UghStreams::Channel(Stream, Node(I, J, K)), ChannelSmooth);
				}
			}
		});
	}
}
