#include "UghStoneDrop.h"

double UghSling::Room(const ugh_logic* Logic, double Middle, double Bottom)
{
	const int32 From = FMath::FloorToInt32(Bottom);
	int32 Found = From + Look;
	for (int32 Column = FMath::FloorToInt32(Middle - HalfColumns); Column < FMath::CeilToInt32(Middle + HalfColumns);
		++Column)
	{
		for (int32 Row = From; Row < Found; ++Row)
		{
			if (ugh_logic_solid(Logic, Column, Row))
			{
				Found = Row;
				break;
			}
		}
	}
	return FMath::Clamp(Found - Bottom, -1.0, double(Look));
}

int32 UghSling::Side(const ugh_logic* Logic, double Middle, double Bottom, int32 Prefer)
{
	const int32 Ground = FMath::RoundToInt32(Bottom + Room(Logic, Middle, Bottom));   // the row under the stone
	const int32 Over = FMath::CeilToInt32(Rows);
	const double Shift = SideClear / UghShapes::UnitsPerPixel;
	for (const int32 Way : { Prefer < 0 ? -1 : 1, Prefer < 0 ? 1 : -1 })
	{
		// no rock in its way along the ground, ground under it there
		const double To = Middle + Way * Shift;
		const int32 Last = FMath::CeilToInt32(FMath::Max(Middle, To) + HalfColumns);
		bool bFree = true;
		for (int32 Column = FMath::FloorToInt32(FMath::Min(Middle, To) - HalfColumns); bFree && Column < Last; ++Column)
		{
			for (int32 Row = Ground - Over; bFree && Row < Ground; ++Row)
			{
				bFree = !ugh_logic_solid(Logic, Column, Row);
			}
		}
		if (bFree && Room(Logic, To, Ground) <= Step)
		{
			return Way;
		}
	}
	return 0;
}

FUghSlingPose UghSling::Pose(double Sway, double Room, const FVector2D& Push)
{
	using namespace UghCopterModel;
	FUghSlingPose Pose;
	Pose.Turn = FQuat(FVector::YAxisVector, Sway);
	Pose.Stone = Pose.Turn.RotateVector(Hanging);
	// its lowest point (a bottom corner when it sways) on the ground at most
	const double Lowest = Pose.Stone.Z - StoneHalfWidth * FMath::Abs(FMath::Sin(Sway));
	Pose.Lift = FMath::Max(0.0, -Room * UghShapes::UnitsPerPixel - Lowest);
	Pose.Stone += FVector(Push.X, Push.Y, Pose.Lift);
	Pose.Sling = Pose.Stone - Pose.Turn.RotateVector(Hanging);
	Pose.Knot = Pose.Sling + Pose.Turn.RotateVector(SlingKnot);
	return Pose;
}

double UghSling::Pushed(const FUghSlingPose& Pose)
{
	return FMath::SmoothStep(PushFrom, PushTo, Pose.Stone.Z + UghCopterModel::StoneHeight);
}

FUghSlingPose UghSling::Follow(const ugh_logic* Logic, double Middle, double Bottom, double Sway, FVector2D& Push,
	double Keep)
{
	// as far as the ground under the copter wants, aside where it can (where it was first), else towards the camera
	const double Wanted = Pushed(Pose(Sway, Room(Logic, Middle, Bottom), FVector2D::ZeroVector));
	const int32 Prefer = Push.X < 0 ? -1 : 1;
	const int32 Way = Wanted > 0 ? Side(Logic, Middle, Bottom, Prefer) : Prefer;
	const FVector2D Target = Wanted * (Way != 0 ? FVector2D(Way * SideClear, 0) : FVector2D(0, FrontClear));
	Push = FMath::Lerp(Target, Push, Keep);
	// on the ground under where it is
	return Pose(Sway, Room(Logic, Middle + Push.X / UghShapes::UnitsPerPixel, Bottom), Push);
}

void FUghStoneDrops::Begin()
{
	for (TPair<int32, FDrop>& Each : Drops)
	{
		Each.Value.bSeen = false;
	}
}

double FUghStoneDrops::Below(int32 Index, double Top, bool bFalling, double Room)
{
	if (!bFalling)
	{
		return 0;
	}
	Room = FMath::Max(0.0, Room);
	FDrop* Drop = Drops.Find(Index);
	if (!Drop)
	{
		// let go: it was in the sling, or on the ground under it
		Drop = &Drops.Add(Index, FDrop{ Top, 0, FMath::Min(SlingBelow(), Room), false });
	}
	Drop->bSeen = true;
	Drop->Fallen = FMath::Max(Drop->Fallen, Top - Drop->Start);
	const double Below = Drop->From * FMath::Max(0.0, 1 - Drop->Fallen / (CatchUp * SlingBelow()));
	return FMath::Min(Below, Room);
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
