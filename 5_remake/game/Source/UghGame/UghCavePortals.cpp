#include "UghCavePortals.h"

#include "UghRockNoise.h"
#include "UghRockOutline.h"
#include "UghShapes.h"

namespace
{
	using UghRockNoise::SmoothMax;

	/** A door of the drawing is 2 x 2 tiles of 16 x 12 px; its floor is looked for this far below its top. */
	constexpr int32 DoorWidth = 32, DoorHeight = 24, FloorFrom = 12, FloorTo = 36;
	/** How boxy the arch's outline is, how round its opening (the powers of their superellipses). */
	constexpr double OutlinePower = 3, OpeningPower = 2.4;
	/**
	 * The arch's front recedes this far towards its rim (pixels); deeper in it widens and rises this much, melting into
	 * the back wall; under the floor it reaches Footing down (the ledge under it is rock anyway).
	 */
	constexpr double Recede = 4, Widen = 14, Rise = 6, Footing = 6;
	/** Its fractured blocks: about this wide and high (pixels), standing out or back up to half of Rough. */
	constexpr double BlockWidth = 7, BlockHeight = 5, Rough = 3;
	/** How ragged the arch's outline is, of its size; its opening's edge, pixels. */
	constexpr double Ragged = 0.12, Jagged = 0.8;
	/** Deeper in the passage turns aside this many of its half widths and narrows to this part of its size. */
	constexpr double TurnAside = 1.3, Narrow = 0.8;
	/** It begins this far in front of the arch's middle (through its blocks); its turn begins TurnFrom behind it. */
	constexpr double Mouth = 3, TurnFrom = 10;
	/** In the passage it is dark from Front + DarkFrom on, as dark as it gets DarkTo deeper. */
	constexpr double DarkFrom = 1, DarkTo = 16;
	/** How smoothly the arch meets the rock, the passage the arch, pixels. */
	constexpr double Smooth = 1.5;

	/** How far out of a superellipse of half sizes A (across) and B (up) a point is: 0 in its middle, 1 on it. */
	double Radius(double Across, double Up, double A, double B, double Power)
	{
		return FMath::Pow(FMath::Pow(FMath::Abs(Across) / A, Power) + FMath::Pow(FMath::Max(Up, 0.0) / B, Power),
			1 / Power);
	}
}

FBox2D FUghCavePortal::Passage() const
{
	const double Aside = TurnAside * HalfWidth;
	return FBox2D(FVector2D(X - HalfWidth - (Turn < 0 ? Aside : 0) - 1, Floor - Height - 1),
		FVector2D(X + HalfWidth + (Turn > 0 ? Aside : 0) + 1, Floor));
}

FBox2D FUghCavePortal::Reach() const
{
	const double Across = HalfWidth + Arch + Widen + 1, Up = Height + Arch + Rise + 1;
	return FBox2D(FVector2D(X - Across, Floor - Up), FVector2D(X + Across, Floor + Footing + 1));
}

double FUghCavePortal::Darkness(const FVector& Point) const
{
	return Passage().IsInside(FVector2D(Point)) ? FMath::SmoothStep(Front + DarkFrom, Front + DarkTo, Point.Z) : 0;
}

float FUghCavePortal::Shape(const FVector& Point, float Field) const
{
	const double Depth = Point.Z;
	if (Depth < Nearest)
	{
		return Field;
	}
	const double Across = Point.X - X, Up = Floor - Point.Y;
	const UghRockNoise::FBlock Block = UghRockNoise::Blocks(Point.X, Point.Y, BlockWidth, BlockHeight, 11);
	const double Fracture = Rough * (Block.Height - 0.5) * (0.4 + 0.6 * Block.Crack);
	// the arch: rock in its outline (wider and higher deeper in) from its front back, the front receding to its rim
	const double Back = FMath::SmoothStep(Front, End, Depth);
	const double Wide = HalfWidth + Arch + Widen * Back, High = Height + Arch + Rise * Back;
	const double Out = Radius(Across, Up, Wide, High, OutlinePower) + Ragged * (Block.Height - 0.5);
	const double ArchFront = Front + Recede * Out * Out - Fracture;
	const double Arched = FMath::Min(FMath::Min((1 - Out) * FMath::Min(Wide, High), Depth - ArchFront), Up + Footing);
	const double Value = SmoothMax(Field, Arched, Smooth);
	// the passage: air in the opening (turning aside and narrowing deeper in), above the floor, to its end
	const double In = FMath::SmoothStep(Front + TurnFrom, End, Depth);
	const double Scale = FMath::Lerp(1.0, Narrow, In);
	const double Open = Radius(Across - Turn * TurnAside * HalfWidth * In, Up, HalfWidth * Scale, Height * Scale,
		OpeningPower);
	const double Air = FMath::Min(FMath::Min((1 - Open) * HalfWidth * Scale + Jagged * Fracture, Up),
		FMath::Min(Depth - (Front - Mouth), End - Depth));
	return -SmoothMax(-Value, Air, Smooth);
}

TArray<FUghCavePortal> UghCavePortals::Plan(const FUghRockOutline& Outline, TConstArrayView<FUghArtTile> Doors)
{
	TArray<FUghCavePortal> Portals;
	for (const FUghArtTile& Door : Doors)
	{
		FUghCavePortal& Portal = Portals.AddDefaulted_GetRef();
		Portal.X = Door.At.X + DoorWidth / 2;
		Portal.Floor = Door.At.Y + DoorHeight;
		const int32 Column = Door.At.X + DoorWidth / 2 - 1 + FUghRockOutline::MarginX;
		for (int32 Y = Door.At.Y + FloorFrom; Y <= Door.At.Y + FloorTo; ++Y)
		{
			const int32 Row = Y + FUghRockOutline::MarginY;
			if (Row >= 0 && Row < FUghRockOutline::Height && Outline.Solid(Column, Row))
			{
				Portal.Floor = Y;
				break;
			}
		}
		// towards the nearer side of the screen
		Portal.Turn = Portal.X < UghShapes::ScreenWidth / 2 ? -1 : 1;
	}
	return Portals;
}
