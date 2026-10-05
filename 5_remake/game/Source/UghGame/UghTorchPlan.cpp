// Where a level's torches are (a part of UghDecorations::Plan).
#include "UghCavePortals.h"
#include "UghDecorations.h"
#include "UghGround.h"
#include "UghLedges.h"
#include "UghPlacer.h"
#include "UghPlans.h"

namespace
{
	/**
	 * A torch's box (pixels: as deep as wide; its shaft, leaning out of the wall, and its flame); how many a level has
	 * at most, how many of them on the back wall.
	 */
	constexpr double TorchWidth = 3, TorchHeight = 11;
	constexpr int32 MaxTorches = 4, WallTorches = 2;
	/** Beside an entrance: this far out from the side of its passage, its shaft's foot this high above its floor. */
	constexpr double Beside = 5, Above = 7;
	/**
	 * On the back wall: above a ledge at least WallLedge long with WallRoom above it, its shaft's foot OverLedge above
	 * it, somewhere in its middle half; at least Apart from the campfires and the other torches.
	 */
	constexpr int32 WallLedge = 20, WallRoom = 24;
	constexpr double OverLedge = 9, Apart = 50;
}

void UghPlans::AddTorches(FUghPlacer& Placer, FRandomStream& Random)
{
	int32 Count = 0;
	auto Try = [&](double X, double Y)
	{
		FUghDecoration Torch;
		Torch.Kind = FUghDecoration::EKind::Torch;
		Torch.X = X;
		Torch.Y = Y;
		Torch.Width = TorchWidth;
		Torch.Height = TorchHeight;
		// the wall is looked for from where it may be nearest (FUghPlacer settles it into the wall)
		Torch.Depth = UghDecorations::SweepReach + TorchWidth * UghShapes::UnitsPerPixel / 2;
		Torch.Yaw = Random.FRandRange(0, 360);
		Torch.Variant = Random.RandHelper(MAX_int16);
		const bool bPlaced = Count < MaxTorches && Placer.TryAdd(Torch);
		Count += bPlaced;
		return bPlaced;
	};
	// beside the entrances: away from where their passages turn, the other side too where there are few
	const TArray<FUghCavePortal>& Portals = Placer.GetGround().GetPortals();
	for (const bool bTurnSide : { false, true })
	{
		for (const FUghCavePortal& Portal : Portals)
		{
			const FBox2D Passage = Portal.Passage();
			const bool bRight = (Portal.Turn > 0) == bTurnSide;
			if (!bTurnSide || Portals.Num() <= MaxTorches / 2)
			{
				Try(bRight ? Passage.Max.X + Beside : Passage.Min.X - Beside, Portal.Floor - Above);
			}
		}
	}
	// on the back wall above the longest ledges, away from the fires
	TArray<FUghLedge> Ledges =
		UghLedges::Find(Placer.GetGround().GetLogic(), Placer.GetWaterRow(), WallRoom, 0);
	Ledges.RemoveAll([](const FUghLedge& Ledge) { return Ledge.Length() < WallLedge; });
	Ledges.StableSort([](const FUghLedge& A, const FUghLedge& B) { return A.Length() > B.Length(); });
	int32 OnWalls = 0;
	for (const FUghLedge& Ledge : Ledges)
	{
		const double X = Ledge.First + Ledge.Length() * Random.FRandRange(0.25, 0.75), Y = Ledge.Y - OverLedge;
		const bool bNear = Placer.GetPlaced().ContainsByPredicate([&](const FUghDecoration& Other)
		{
			return (Other.Kind == FUghDecoration::EKind::Campfire || Other.Kind == FUghDecoration::EKind::Torch) &&
				FMath::Abs(Other.X - X) < Apart && FMath::Abs(Other.Y - Y) < Apart;
		});
		if (OnWalls < WallTorches && !bNear && Try(X, Y))
		{
			++OnWalls;
		}
	}
}
