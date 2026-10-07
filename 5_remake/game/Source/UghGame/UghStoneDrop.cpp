#include "UghStoneDrop.h"

void FUghStoneDrops::Begin()
{
	for (TPair<int32, FDrop>& Each : Drops)
	{
		Each.Value.bSeen = false;
	}
}

double FUghStoneDrops::Below(int32 Index, double Top, bool bFalling)
{
	if (!bFalling)
	{
		return 0;
	}
	FDrop* Drop = Drops.Find(Index);
	if (!Drop)
	{
		Drop = &Drops.Add(Index, FDrop{ Top, 0, false });   // let go: it was in the sling
	}
	Drop->bSeen = true;
	Drop->Fallen = FMath::Max(Drop->Fallen, Top - Drop->Start);
	const double Below = SlingBelow();
	return Below * FMath::Max(0.0, 1 - Drop->Fallen / (CatchUp * Below));
}

void FUghStoneDrops::End()
{
	for (auto It = Drops.CreateIterator(); It; ++It)
	{
		if (!It.Value().bSeen)
		{
			It.RemoveCurrent();
		}
	}
}
