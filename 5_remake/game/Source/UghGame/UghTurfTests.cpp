// The turf of every level as an automation test of the editor (Ugh.Turf): the grass over the edges never above a
// surface nor over the air of the play, the paths at the cave entrances.
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
#include "UghTurf.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };
	/** Every level has at least this many cards of grass. */
	constexpr int32 MinCards = 40;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghTurfTest, "Ugh.Turf", EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

/**
 * In every level the grass hanging over the edges is in front of the slab of the play and only ever over rock of the
 * collision mask (at or below a surface: it never hides a figure's feet, a pad's surface or the air the figures fly
 * in), none where a path is trodden bare, the same every time; a path in front of every cave entrance.
 */
bool FUghTurfTest::RunTest(const FString& Parameters)
{
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghLevelArt Art;
	TSharedPtr<FJsonObject> Levels;
	FString Error;
	const FString LevelsPath = Assets / UghJson::LevelsFile;
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error) &&
		Sprites.Load(Assets, Error) && UghJson::ReadObject(LevelsPath, Levels, Error) &&
		Art.Load(*Levels, LevelsPath, Error)))
	{
		return false;
	}
	int32 Cards = 0, Paths = 0;
	for (const auto& [Players, Count] : Modes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const ugh_logic_view& View = Simulation.GetCurrent();
			const FString Name =
				FString::Printf(TEXT("%s level %d"), Players == 1 ? TEXT("one player") : TEXT("team"), Level + 1);
			const int32 WaterRow = View.water_level / UghShapes::Subpixels;
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(View.level_id, Sprites), Art.Doors(View.level_id));
			const TArray<FUghPadSign> Signs = UghPadSigns::Plan(Logic, FUghGround(Logic, Field), Art.Signs(View.level_id));
			const TArray<FUghStream> Streams = UghStreams::Plan(Logic, Field, WaterRow, Signs);
			Field.CarveChannels(Streams);
			FUghTurf Turf;
			Turf.Build(Logic, Field, WaterRow, Streams);
			FUghTurf Again;
			Again.Build(Logic, Field, WaterRow, Streams);
			TestTrue(Name + TEXT(": the same turf every time"), Turf.GetPaths() == Again.GetPaths() &&
				Turf.GetBlades().Vertices == Again.GetBlades().Vertices);
			TestTrue(FString::Printf(TEXT("%s: grass over the edges (%d cards)"), *Name, Turf.CardCount()),
				Turf.CardCount() >= MinCards);
			Cards += Turf.CardCount();
			const FUghRockMesh& Blades = Turf.GetBlades();
			int32 Wrong = 0;
			FString First;
			for (int32 Index = 0; Index < Blades.Vertices.Num(); ++Index)
			{
				const FVector& Point = Blades.Vertices[Index];
				const double X = Point.X / UghShapes::UnitsPerPixel;
				const double Y = UghShapes::ScreenHeight - Point.Z / UghShapes::UnitsPerPixel;
				const double Depth = -Point.Y / UghShapes::UnitsPerPixel;
				// a point on a pixel's border belongs to the pixel inside the card (below it, towards the card's middle)
				const int32 Column = FMath::FloorToInt32(Blades.UVs[Index].X > 0.5 ? X - 1e-4 : X + 1e-4);
				const int32 Row = FMath::FloorToInt32(Y + 1e-4);
				FString Problem;
				if (Depth >= -FUghRockField::SlabHalf)
				{
					Problem = TEXT("in the slab of the play");
				}
				else if (!ugh_logic_solid(Logic, Column, Row))
				{
					Problem = TEXT("over the air of the play");
				}
				else if (Blades.Colors[Index].B >= 0.8 * 255)
				{
					Problem = TEXT("on a path trodden bare");
				}
				if (!Problem.IsEmpty() && Wrong++ == 0)
				{
					First = FString::Printf(TEXT("%.2f, %.2f, %.2f: %s"), X, Y, Depth, *Problem);
				}
			}
			TestTrue(FString::Printf(TEXT("%s: the grass only over rock in front of the slab (%d wrong, %s)"), *Name,
				Wrong, *First), Wrong == 0);
			for (const FUghCavePortal& Portal : Field.GetPortals())
			{
				const int32 Column = FMath::FloorToInt32(Portal.X - 0.5), Row = FMath::RoundToInt32(Portal.Floor);
				if (FUghTurf::Edge(Logic, Column, Row) && Row < WaterRow)
				{
					++Paths;
					TestEqual(FString::Printf(TEXT("%s: a path at the entrance at %.0f, %.0f"), *Name, Portal.X,
						Portal.Floor), int32(Turf.PathAt(Column, Row)), 255);
				}
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d cards of grass, %d paths"), Cards, Paths));
	TestTrue(TEXT("paths at the entrances"), Paths > 20);
	return true;
}

#endif
