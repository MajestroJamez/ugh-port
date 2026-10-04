// The lianas of a level (UghPlans).
#include "UghGround.h"
#include "UghPlacer.h"
#include "UghPlans.h"
#include "UghShapes.h"

namespace
{
	/** A liana hangs from about every VineEvery-th pixel of a ceiling with at least VineRoom pixels of air below. */
	constexpr int32 VineEvery = 3, VineRoom = 10;
	/**
	 * How long (pixels; at most VineReach of the room below) and how wide for its length; each at most VineBack units
	 * further back than it must be.
	 */
	constexpr double VineMin = 6, VineMax = 44, VineReach = 0.6, VineAspect = 0.3, VineWidthMin = 3, VineBack = 80;
	/**
	 * Lianas down the back wall: from the top of every CreeperStep-th column of the screen's air (a ceiling of the
	 * mask or the screen's top) in patches (where a noise along the screen is above CreeperPatch, PatchScale per
	 * pixel), CreeperBelow pixels below it (the wall reaches out to the ceiling there); pieces CreeperMin ..
	 * CreeperMax pixels long, each going on with a next one below it (overlapping CreeperOverlap) by CreeperOn,
	 * down to at most CreeperReach of the room below.
	 */
	constexpr double CreeperStep = 3, CreeperPatch = 0.05, PatchScale = 0.05, CreeperBelow = 6;
	constexpr double CreeperMin = 10, CreeperMax = 30, CreeperOverlap = 2, CreeperOn = 0.7, CreeperReach = 0.85;
	/** The lengths (pixels) the meshes of the vines are made for, from the shortest (Blender/vines.py: 1.2 .. 3.6 m). */
	constexpr double VineLengths[] = { 12, 20, 28, 36 };

	int32 RoomBelow(const FUghGround& Ground, int32 X, int32 Y)
	{
		int32 Room = 0;
		while (Y + Room < UghShapes::ScreenHeight && !Ground.Solid(X, Y + Room))
		{
			++Room;
		}
		return Room;
	}

	/** The mesh of a liana as long as `Length` (pixels): the nearest of VineLengths. */
	int32 VariantFor(double Length)
	{
		int32 Best = 0;
		for (int32 Index = 1; Index < UE_ARRAY_COUNT(VineLengths); ++Index)
		{
			if (FMath::Abs(VineLengths[Index] - Length) < FMath::Abs(VineLengths[Best] - Length))
			{
				Best = Index;
			}
		}
		return Best;
	}
}

void UghPlans::AddVines(FUghPlacer& Placer, FRandomStream& Random)
{
	const FUghGround& Ground = Placer.GetGround();
	for (int32 Y = 1; Y < FMath::Min(Placer.GetWaterRow(), UghShapes::ScreenHeight); ++Y)
	{
		for (int32 X = 0; X < UghShapes::ScreenWidth; ++X)
		{
			// under a ceiling of the mask, now and then
			if (!Ground.Solid(X, Y - 1) || Ground.Solid(X, Y) || Random.RandHelper(VineEvery) != 0)
			{
				continue;
			}
			const int32 Room = FMath::Min(RoomBelow(Ground, X, Y), Placer.GetWaterRow() - Y);
			if (Room < VineRoom)
			{
				continue;
			}
			FUghDecoration Vine;
			Vine.Kind = FUghDecoration::EKind::Vine;
			Vine.Height = Random.FRandRange(VineMin, FMath::Min(VineMax, Room * VineReach));
			Vine.Width = FMath::Max(VineWidthMin, Vine.Height * VineAspect);
			Vine.X = X + 0.5;
			Vine.Y = Y;
			Vine.Yaw = Random.FRandRange(-40, 40);   // its leaves towards the camera
			Vine.Variant = VariantFor(Vine.Height);
			Placer.TryAddBehind(Vine, Random.FRandRange(0, VineBack));
		}
	}
}

void UghPlans::AddCreepers(FUghPlacer& Placer, FRandomStream& Random)
{
	const FUghGround& Ground = Placer.GetGround();
	const double Patches = Random.FRandRange(0, 1000);
	for (double X = CreeperStep / 2; X < UghShapes::ScreenWidth; X += CreeperStep)
	{
		const int32 Column = FMath::FloorToInt32(X);
		for (int32 Y = 0; Y < FMath::Min(Placer.GetWaterRow(), UghShapes::ScreenHeight); ++Y)
		{
			// the top of a run of air in the column (under a ceiling or the screen's top: the cliff goes on), in a patch
			if ((Y > 0 && !Ground.Solid(Column, Y - 1)) || Ground.Solid(Column, Y) ||
				FMath::PerlinNoise1D(Patches + X * PatchScale + Y * 0.37) < CreeperPatch)
			{
				continue;
			}
			const int32 Room = FMath::Min(RoomBelow(Ground, Column, Y), Placer.GetWaterRow() - Y);
			if (Room * CreeperReach < CreeperMin + CreeperBelow)
			{
				continue;
			}
			// a liana, now and then going on further down (pieces one below the other) while there is room
			const double Bottom = Y + Room * CreeperReach;
			double Top = Y + CreeperBelow;
			const double At = X + Random.FRandRange(-0.5, 0.5);
			do
			{
				FUghDecoration Creeper;
				Creeper.Kind = FUghDecoration::EKind::Creeper;
				Creeper.Height = Random.FRandRange(CreeperMin, CreeperMax);
				Creeper.Width = FMath::Max(VineWidthMin, Creeper.Height * VineAspect);
				Creeper.X = At;
				Creeper.Y = Top;
				Creeper.Yaw = Random.FRandRange(-25, 25);   // its leaves towards the camera
				Creeper.Variant = VariantFor(Creeper.Height);
				if (Creeper.Bottom() > Bottom || !Placer.TryAddBehind(Creeper, 0))
				{
					break;
				}
				Top = Creeper.Bottom() - CreeperOverlap;
			}
			while (Random.FRand() < CreeperOn);
		}
	}
}
