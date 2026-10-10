// The roots of the trees of every level as an automation test of the editor (Ugh.TreeRoots): down the rock's face,
// never over the air of the play.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghGround.h"
#include "UghJson.h"
#include "UghLevelArt.h"
#include "UghPadSigns.h"
#include "UghRockField.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghStreams.h"
#include "UghTreeRoots.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 RootModes[][2] = { { 1, 69 }, { 2, 81 } };
	/** Pixels a root may reach past a pixel of rock (its line smoothed across). */
	constexpr double RootSlack = 0.35;
	/** A tree this close above the water at the level's start (pixels) may have no roots. */
	constexpr double AtTheWater = 6;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghTreeRootsTest, "Ugh.TreeRoots", EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

/**
 * Every tree of the data stands on the ground (its foot at a ledge's surface); nearly every one not just above the
 * water has roots down the rock's face, long ones in most levels; each root only ever over rock of the collision mask
 * (its whole width and height, never above a surface: never over a figure's feet or the air of the play), its front in
 * front of the slab of the play; the same every time.
 */
bool FUghTreeRootsTest::RunTest(const FString& Parameters)
{
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghLevelArt Art;
	FUghTreePlaces Places;
	TSharedPtr<FJsonObject> Levels;
	FString Error;
	const FString LevelsPath = Assets / UghJson::LevelsFile;
	const FString Data = Assets / TEXT("logic/ugh-data.ugd");
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Data, Error) && Places.Load(Data, Error) &&
		Sprites.Load(Assets, Error) && UghJson::ReadObject(LevelsPath, Levels, Error) &&
		Art.Load(*Levels, LevelsPath, Error)))
	{
		return false;
	}
	int32 Trees = 0, Rooted = 0, Strands = 0, Long = 0, Triangles = 0;
	for (const auto& [Players, Count] : RootModes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const ugh_logic_view& View = Simulation.GetCurrent();
			const TArray<FVector2D> Feet = Places.Of(View.level_id);
			if (Feet.IsEmpty())
			{
				continue;
			}
			const FString Name =
				FString::Printf(TEXT("%s level %d"), Players == 1 ? TEXT("one player") : TEXT("team"), Level + 1);
			for (const FVector2D& Foot : Feet)
			{
				const int32 X = FMath::FloorToInt32(Foot.X), Y = FMath::RoundToInt32(Foot.Y);
				bool bGround = false;
				for (int32 Row = Y - 2; Row <= Y + 2; ++Row)
				{
					bGround |= ugh_logic_solid(Logic, X, Row) && !ugh_logic_solid(Logic, X, Row - 1);
				}
				TestTrue(FString::Printf(TEXT("%s: the tree at %.0f, %.0f stands on the ground"), *Name, Foot.X, Foot.Y),
					bGround);
			}
			const int32 WaterRow = View.water_level / UghShapes::Subpixels;
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(View.level_id, Sprites), Art.Doors(View.level_id));
			const TArray<FUghPadSign> Signs = UghPadSigns::Plan(Logic, FUghGround(Logic, Field), Art.Signs(View.level_id));
			const TArray<FUghStream> Streams = UghStreams::Plan(Logic, Field, WaterRow, Signs);
			Field.CarveChannels(Streams);
			FUghTreeRoots Roots, Again;
			Roots.Build(Logic, Field, Feet, WaterRow, Streams, View.level_id);
			Again.Build(Logic, Field, Feet, WaterRow, Streams, View.level_id);
			TestTrue(Name + TEXT(": the same roots every time"), Roots.GetMesh().Vertices == Again.GetMesh().Vertices);
			Triangles += Roots.GetMesh().Triangles.Num() / 3;
			for (const FVector2D& Foot : Feet)
			{
				if (Foot.Y >= WaterRow - AtTheWater)
				{
					continue;   // (its ledge just above the water: no room)
				}
				++Trees;
				const bool bRooted = Roots.GetStrands().ContainsByPredicate([&Foot](const FUghTreeRoots::FStrand& Strand)
				{
					return FMath::Abs(Strand.Points[0].X - Foot.X) <= FUghTreeRoots::Spread + 2;
				});
				Rooted += bRooted ? 1 : 0;
				if (!bRooted)
				{
					AddInfo(FString::Printf(TEXT("%s: no roots for the tree at %.0f, %.0f (water at %d)"), *Name, Foot.X,
						Foot.Y, WaterRow));
				}
			}
			int32 Wrong = 0;
			FString First;
			for (const FUghTreeRoots::FStrand& Strand : Roots.GetStrands())
			{
				++Strands;
				Long += Strand.Points.Last().Y - Strand.Points[0].Y >= 30 ? 1 : 0;
				for (int32 Index = 0; Index < Strand.Points.Num(); ++Index)
				{
					const FVector& Point = Strand.Points[Index];
					const double Radius = Strand.Radii[Index] - RootSlack;
					FString Problem;
					for (int32 Row = FMath::FloorToInt32(Point.Y - Radius); Row <= FMath::FloorToInt32(Point.Y + Radius);
						++Row)
					{
						for (int32 Column = FMath::FloorToInt32(Point.X - Radius);
							Column <= FMath::FloorToInt32(Point.X + Radius); ++Column)
						{
							Problem = ugh_logic_solid(Logic, Column, Row) ? Problem : TEXT("over the air of the play");
						}
					}
					if (Point.Z - Strand.Radii[Index] > -FUghRockField::SlabHalf && Index > 0 &&
						Index + 1 < Strand.Points.Num())
					{
						Problem = TEXT("in the slab of the play");
					}
					if (!Problem.IsEmpty() && Wrong++ == 0)
					{
						First = FString::Printf(TEXT("%.2f, %.2f, %.2f r %.2f: %s"), Point.X, Point.Y, Point.Z,
							Strand.Radii[Index], *Problem);
					}
				}
			}
			TestTrue(FString::Printf(TEXT("%s: the roots only over rock in front of the slab (%d wrong, %s)"), *Name,
				Wrong, *First), Wrong == 0);
		}
	}
	AddInfo(FString::Printf(TEXT("%d trees above the water, %d with roots; %d roots, %d of them 30 px or longer; %d triangles"), Trees,
		Rooted, Strands, Long, Triangles));
	TestTrue(TEXT("trees in the levels"), Trees > 100);
	TestTrue(TEXT("nearly every tree has roots"), Rooted >= 0.9 * Trees);
	TestTrue(TEXT("long roots"), Long >= Trees);
	return true;
}

#endif
