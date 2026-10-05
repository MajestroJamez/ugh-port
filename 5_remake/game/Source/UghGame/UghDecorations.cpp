#include "UghDecorations.h"

#include "UghGround.h"
#include "UghLedges.h"
#include "UghPlacer.h"
#include "UghPlans.h"
#include "UghRockField.h"
#include "ugh_logic.h"

namespace
{
	using FDecoration = FUghDecoration;
	using EKind = FUghDecoration::EKind;

	constexpr int32 ScreenWidth = UghShapes::ScreenWidth;

	/** Campfires: how many, their box (pixels: the stones and the flame), the ledge they need, how far apart. */
	constexpr int32 CampfireCount = 3;
	constexpr double CampfireWidth = 12, CampfireHeight = 12;
	constexpr int32 CampfireLedge = 14, CampfireRoom = 16, CampfireApart = 40;
	/**
	 * Palms, pixels: how many, at least and at most this tall, PalmAspect as wide as tall but at most PalmCanopy (so
	 * that the crown fits in the cave), with this much room above; only the tallest that fit are chosen from (at least
	 * PalmPreferred of the tallest), at least PalmApart apart; PalmSmall .. Middle tall where none of those fits.
	 */
	constexpr int32 PalmCount = 4, PalmMin = 16, PalmMax = 48, PalmSmall = 10, PalmCanopy = 26, PalmHeadroom = 2;
	constexpr int32 PalmApart = 30;
	constexpr double PalmAspect = 0.7, PalmPreferred = 0.75;
	/** Totems and huts: how many, how many tries for them, how tall (pixels) and how wide for their height. */
	constexpr int32 TotemCount = 2, HutCount = 1, LandmarkTries = 40;
	constexpr double TotemMin = 12, TotemMax = 20, TotemAspect = 0.6, HutMin = 10, HutMax = 15, HutAspect = 1.4;
	/** They are turned at most this much from the camera, degrees. */
	constexpr double LandmarkTurn = 35;
	/** Each is put this many units further back than the nearest it may be, at most. */
	constexpr double BackMax = 60;

	FDecoration Make(EKind Kind, double X, double Y, double Width, double Height, FRandomStream& Random)
	{
		FDecoration Decoration;
		Decoration.Kind = Kind;
		Decoration.X = X;
		Decoration.Y = Y;
		Decoration.Width = Width;
		Decoration.Height = Height;
		Decoration.Yaw = Random.FRandRange(0, 360);
		Decoration.Variant = Random.RandHelper(MAX_int16);
		return Decoration;
	}

	/**
	 * The tallest palm (pixels, 0: none) of MinHeight .. MaxHeight whose crown fits over column X of a row whose
	 * columns have `Rooms`.
	 */
	int32 TallestPalm(const TArray<int32>& Rooms, int32 X, int32 MinHeight, int32 MaxHeight)
	{
		for (int32 Height = MaxHeight; Height >= MinHeight; --Height)
		{
			const int32 Width = FMath::Min(PalmCanopy, FMath::FloorToInt32(Height * PalmAspect)), Left = X - Width / 2;
			bool bFits = Left >= 0 && Left + Width <= ScreenWidth;
			for (int32 Column = Left; bFits && Column < Left + Width; ++Column)
			{
				bFits = Rooms[Column] >= Height + PalmHeadroom;
			}
			if (bFits)
			{
				return Height;
			}
		}
		return 0;
	}

	/** Palms of MinHeight .. MaxHeight (the tallest that fit, apart from each other); how many were planted. */
	int32 AddPalmsOf(FUghPlacer& Placer, FRandomStream& Random, int32 MinHeight, int32 MaxHeight)
	{
		const ugh_logic* Logic = Placer.GetGround().GetLogic();
		TArray<FDecoration> Candidates;
		int32 Tallest = 0;
		for (const FUghLedge& Ledge :
			UghLedges::Find(Logic, Placer.GetWaterRow(), MinHeight + PalmHeadroom, UghLedges::PadsToo))
		{
			TArray<int32> Rooms;
			for (int32 Column = 0; Column < ScreenWidth; ++Column)
			{
				Rooms.Add(UghLedges::RoomAbove(Logic, Column, Ledge.Y, MaxHeight + PalmHeadroom));
			}
			for (int32 X = Ledge.First + 1; X < Ledge.Last - 1; X += 2)
			{
				if (const int32 Height = TallestPalm(Rooms, X, MinHeight, MaxHeight))
				{
					const double Width = FMath::Min(double(PalmCanopy), Height * PalmAspect);
					Candidates.Add(Make(EKind::Palm, X + 0.5, Ledge.Y, Width, Height, Random));
					Tallest = FMath::Max(Tallest, Height);
				}
			}
		}
		Candidates.RemoveAll([&](const FDecoration& Palm) { return Palm.Height < Tallest * PalmPreferred; });
		int32 Planted = 0;
		while (Planted < PalmCount && !Candidates.IsEmpty())
		{
			const FDecoration Palm = Candidates[Random.RandHelper(Candidates.Num())];
			Candidates.RemoveAll([&](const FDecoration& Other)
			{
				return FMath::Abs(Other.X - Palm.X) < PalmApart && FMath::Abs(Other.Y - Palm.Y) < PalmApart;
			});
			Planted += Placer.TryAddBehind(Palm, Random.FRandRange(0, BackMax));
		}
		return Planted;
	}
}

const TCHAR* UghDecorations::Name(FUghDecoration::EKind Kind)
{
	static const TCHAR* const Names[] = { TEXT("grass"), TEXT("flower"), TEXT("rock"), TEXT("bones"), TEXT("fern"),
		TEXT("bush"), TEXT("plant"), TEXT("stump"), TEXT("palm"), TEXT("totem"), TEXT("hut"), TEXT("vine"),
		TEXT("creeper"), TEXT("campfire") };
	static_assert(UE_ARRAY_COUNT(Names) == int32(EKind::Campfire) + 1);
	return Names[int32(Kind)];
}

FString UghDecorations::Summary(const TArray<FUghDecoration>& Decorations)
{
	TArray<int32> Counts;
	Counts.Init(0, int32(EKind::Campfire) + 1);
	for (const FDecoration& Decoration : Decorations)
	{
		++Counts[int32(Decoration.Kind)];
	}
	TArray<FString> Parts;
	for (int32 Kind = 0; Kind < Counts.Num(); ++Kind)
	{
		Parts.Add(FString::Printf(TEXT("%s %d"), Name(EKind(Kind)), Counts[Kind]));
	}
	return FString::Join(Parts, TEXT(", "));
}

double UghDecorations::NearestFront(const FUghDecoration& Decoration)
{
	if (Decoration.Hangs() || Decoration.Height > Middle)
	{
		return SweepReach;
	}
	return Decoration.Height > GroundCover ? FigureReach : SlabFront;
}

void UghPlans::AddCampfires(FUghPlacer& Placer, FRandomStream& Random)
{
	const ugh_logic* Logic = Placer.GetGround().GetLogic();
	TArray<FUghLedge> Ledges = UghLedges::Find(Logic, Placer.GetWaterRow(), CampfireRoom, 0);
	Ledges.RemoveAll([](const FUghLedge& Ledge) { return Ledge.Length() < CampfireLedge; });
	Ledges.StableSort([](const FUghLedge& A, const FUghLedge& B) { return A.Length() > B.Length(); });
	TArray<FDecoration> Lit;
	for (const FUghLedge& Ledge : Ledges)
	{
		const double X = (Ledge.First + Ledge.Last) / 2.0;
		const bool bNear = Lit.ContainsByPredicate([&](const FDecoration& Fire)
		{
			return FMath::Abs(Fire.X - X) < CampfireApart && FMath::Abs(Fire.Y - Ledge.Y) < CampfireApart;
		});
		if (Lit.Num() < CampfireCount && !bNear &&
			Placer.TryAddBehind(Make(EKind::Campfire, X, Ledge.Y, CampfireWidth, CampfireHeight, Random),
				Random.FRandRange(0, BackMax / 2)))
		{
			Lit.Add(Placer.GetPlaced().Last());
		}
	}
}

void UghPlans::AddPalms(FUghPlacer& Placer, FRandomStream& Random)
{
	// where no tall one fits (as deep as it must be), small ones (nearer: no rotor reaches their crowns)
	if (AddPalmsOf(Placer, Random, PalmMin, PalmMax) == 0)
	{
		AddPalmsOf(Placer, Random, PalmSmall, int32(UghDecorations::Middle));
	}
}

void UghPlans::AddLandmarks(FUghPlacer& Placer, FRandomStream& Random)
{
	const ugh_logic* Logic = Placer.GetGround().GetLogic();
	const TArray<FUghLedge> Ledges = UghLedges::Find(Logic, Placer.GetWaterRow(), HutMin + 1, UghLedges::PadsToo);
	int32 Totems = 0, Huts = 0;
	for (int32 Try = 0; Try < LandmarkTries && !Ledges.IsEmpty() && (Totems < TotemCount || Huts < HutCount); ++Try)
	{
		const FUghLedge& Ledge = Ledges[Random.RandHelper(Ledges.Num())];
		const bool bHut = Huts < HutCount && (Totems >= TotemCount || Random.FRand() < 0.5);
		const double Height = bHut ? Random.FRandRange(HutMin, HutMax) : Random.FRandRange(TotemMin, TotemMax);
		const double Width = Height * (bHut ? HutAspect : TotemAspect);
		const double X = Random.FRandRange(Ledge.First + Width / 2, Ledge.Last - Width / 2);
		if (Ledge.Length() < Width || UghLedges::RoomAbove(Logic, FMath::FloorToInt32(X), Ledge.Y, 64) < Height + 1)
		{
			continue;
		}
		FDecoration Landmark = Make(bHut ? EKind::Hut : EKind::Totem, X, Ledge.Y, Width, Height, Random);
		Landmark.Yaw = Random.FRandRange(-LandmarkTurn, LandmarkTurn);   // its door, its faces towards the camera
		if (Placer.TryAddBehind(Landmark, Random.FRandRange(0, BackMax)))
		{
			(bHut ? Huts : Totems) += 1;
		}
	}
}

TArray<FUghDecoration> UghDecorations::Plan(const ugh_logic* Logic, const FUghRockField& Field, int32 LevelId,
	int32 WaterRow, const TArray<FUghPadSign>& Signs)
{
	if (Field.IsEmpty())
	{
		return {};
	}
	const FUghGround Ground(Logic, Field);
	FUghPlacer Placer(Ground, WaterRow, Signs);
	// each part its own sequence of the level's numbers: a change in one leaves the others as they were
	void (*const Parts[])(FUghPlacer&, FRandomStream&) = { &UghPlans::AddCampfires, &UghPlans::AddPalms,
		&UghPlans::AddLandmarks, &UghPlans::AddPlants, &UghPlans::AddMeadows, &UghPlans::AddVines,
		&UghPlans::AddCreepers };
	for (int32 Part = 0; Part < UE_ARRAY_COUNT(Parts); ++Part)
	{
		FRandomStream Random(LevelId * 101 + Part);
		Parts[Part](Placer, Random);
	}
	return Placer.TakePlaced();
}
