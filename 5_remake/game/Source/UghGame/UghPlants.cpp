// The plants, rocks and bones along the ledges of a level, its meadows and the rocks of its springs (UghPlans).
#include "UghGround.h"
#include "UghLedges.h"
#include "UghPlacer.h"
#include "UghPlans.h"
#include "UghStreams.h"

namespace
{
	using FDecoration = FUghDecoration;
	using EKind = FUghDecoration::EKind;

	/** A kind along the ledges: how often (a weight), how tall (pixels), how wide for its height. */
	struct FAlong
	{
		EKind Kind;
		double Weight, MinHeight, MaxHeight, MinAspect, MaxAspect;
	};
	const FAlong Alongs[] = {
		{ EKind::Bush, 3, 6, 15, 1.2, 1.6 },
		{ EKind::Fern, 3, 6, 13, 1.2, 1.6 },
		{ EKind::Plant, 3, 6, 14, 1.0, 1.3 },
		{ EKind::Rock, 2, 2, 8, 1.4, 2.0 },
		{ EKind::Stump, 0.6, 3, 6, 1.0, 1.1 },
		{ EKind::Bones, 1, 2, 5, 2.5, 3.5 } };
	/**
	 * Along a ledge a place is tried every this many pixels (at random between), with up to AlongTries kinds and
	 * depths, each at most AlongBack further back than it must be.
	 */
	constexpr double AlongStepMin = 2, AlongStepMax = 4, AlongBack = 120;
	constexpr int32 AlongTries = 3;
	/** A ledge needs this much room above it for plants, pixels. */
	constexpr int32 AlongRoom = 5;

	/**
	 * Meadows: a tuft every MeadowStep pixels (give or take a third) in rows MeadowRows units apart from as near as
	 * they may be to MeadowDepth further back; grass this tall (pixels), flowers this tall, as wide as tall times
	 * MeadowAspectMin .. MeadowAspect. Flowers grow in patches (where a noise along the ledges is above FlowerPatch:
	 * FlowerShare of the tufts there, FlowerStray elsewhere).
	 */
	constexpr double MeadowStep = 0.8, MeadowRows = 15, MeadowDepth = 150, MeadowAspectMin = 1.3, MeadowAspect = 2.2;
	constexpr double GrassMin = 2.5, GrassMax = 5, FlowerMin = 2.5, FlowerMax = 4.5;
	constexpr double FlowerPatch = 0.2, FlowerShare = 0.5, FlowerStray = 0.03, PatchScale = 0.07;
	constexpr int32 MeadowRoom = 6;
	/**
	 * A spring's rocks beside its hole, a fern beyond each: their box (pixels), how far from the bank (pixels) and from
	 * the back wall (units).
	 */
	constexpr double SpringRock = 2.6, SpringRockAspect = 1.5, SpringFern = 7, SpringFernAspect = 1.4;
	constexpr double SpringRockOut = 1.6, SpringFernOut = 5, SpringRockFront = 20, SpringFernFront = 45;

	const FAlong& Pick(FRandomStream& Random)
	{
		double Total = 0;
		for (const FAlong& Along : Alongs)
		{
			Total += Along.Weight;
		}
		double Left = Random.FRand() * Total;
		for (const FAlong& Along : Alongs)
		{
			if ((Left -= Along.Weight) < 0)
			{
				return Along;
			}
		}
		return Alongs[0];
	}

	FDecoration Make(EKind Kind, double X, int32 Y, double Height, double Aspect, FRandomStream& Random)
	{
		FDecoration Decoration;
		Decoration.Kind = Kind;
		Decoration.X = X;
		Decoration.Y = Y;
		Decoration.Height = Height;
		Decoration.Width = Height * Aspect;
		Decoration.Yaw = Random.FRandRange(0, 360);
		Decoration.Variant = Random.RandHelper(MAX_int16);
		return Decoration;
	}
}

void UghPlans::AddPlants(FUghPlacer& Placer, FRandomStream& Random)
{
	const ugh_logic* Logic = Placer.GetGround().GetLogic();
	for (const FUghLedge& Ledge : UghLedges::Find(Logic, Placer.GetWaterRow(), AlongRoom, UghLedges::PadsToo))
	{
		for (double X = Ledge.First + Random.FRandRange(0, AlongStepMax); X < Ledge.Last;
			X += Random.FRandRange(AlongStepMin, AlongStepMax))
		{
			const int32 Room = UghLedges::RoomAbove(Logic, FMath::FloorToInt32(X), Ledge.Y, 64);
			for (int32 Try = 0; Try < AlongTries; ++Try)
			{
				const FAlong& Along = Pick(Random);
				const double Height = FMath::Min(Random.FRandRange(Along.MinHeight, Along.MaxHeight), Room - 1.0);
				if (Height >= Along.MinHeight && Placer.TryAddBehind(Make(Along.Kind, X, Ledge.Y, Height,
					Random.FRandRange(Along.MinAspect, Along.MaxAspect), Random), Random.FRandRange(0, AlongBack)))
				{
					break;
				}
			}
		}
	}
}

void UghPlans::AddMeadows(FUghPlacer& Placer, FRandomStream& Random)
{
	const ugh_logic* Logic = Placer.GetGround().GetLogic();
	const double Patches = Random.FRandRange(0, 1000);
	for (const FUghLedge& Ledge : UghLedges::Find(Logic, Placer.GetWaterRow(), MeadowRoom, UghLedges::PadsToo))
	{
		for (double Along = Ledge.First + MeadowStep / 2; Along < Ledge.Last; Along += MeadowStep)
		{
			const bool bPatch = FMath::PerlinNoise1D(Patches + Along * PatchScale + Ledge.Y) > FlowerPatch;
			for (double Row = 0; Row <= MeadowDepth; Row += MeadowRows)
			{
				const bool bFlower = Random.FRand() < (bPatch ? FlowerShare : FlowerStray);
				const double Height = bFlower ? Random.FRandRange(FlowerMin, FlowerMax)
					: Random.FRandRange(GrassMin, GrassMax);
				const double X = Along + Random.FRandRange(-MeadowStep, MeadowStep) / 3;
				Placer.TryAddBehind(Make(bFlower ? EKind::Flower : EKind::Grass, X, Ledge.Y, Height,
					Random.FRandRange(MeadowAspectMin, MeadowAspect), Random), Row + Random.FRandRange(0, MeadowRows / 2));
			}
		}
	}
}

void UghPlans::AddSprings(FUghPlacer& Placer, FRandomStream& Random)
{
	for (const FUghStream& Stream : Placer.GetStreams())
	{
		for (const double Side : { -1.0, 1.0 })
		{
			const double Bank = Side < 0 ? Stream.Left : Stream.Right;
			const double RockWidth = SpringRock * SpringRockAspect, FernWidth = SpringFern * SpringFernAspect;
			FDecoration Rock = Make(EKind::Rock, Bank + Side * (SpringRockOut + RockWidth / 2), Stream.Y, SpringRock,
				SpringRockAspect, Random);
			Rock.Depth = Stream.Spring - SpringRockFront - RockWidth * UghShapes::UnitsPerPixel / 2;
			Placer.TryAdd(Rock);
			FDecoration Fern = Make(EKind::Fern, Bank + Side * (SpringFernOut + FernWidth / 2), Stream.Y, SpringFern,
				SpringFernAspect, Random);
			Fern.Depth = Stream.Spring - SpringFernFront - FernWidth * UghShapes::UnitsPerPixel / 2;
			Placer.TryAdd(Fern);
		}
	}
}
