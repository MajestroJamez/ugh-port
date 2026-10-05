// The cave entrances of every level as an automation test of the editor (Ugh.Portals): one at each door of the
// drawing, open, framed by rock, deep, behind everything that flies past.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghDecorations.h"
#include "UghFigurePlace.h"
#include "UghJson.h"
#include "UghLevelArt.h"
#include "UghRockField.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };
	constexpr double UnitsPerPixel = UghShapes::UnitsPerPixel;

	// the slab of the play stays the mask's (Ugh.Rock), a rotor or a wing never reaches the arch
	static_assert(FUghCavePortal::Nearest > FUghRockField::SlabHalf + 2);
	static_assert(FUghCavePortal::Front * UnitsPerPixel >= UghDecorations::SweepReach);

	/** What is wrong with `Portal` of the door `Door` in `Field` (its level `Logic`); empty when nothing. */
	FString Problem(const FUghCavePortal& Portal, const FUghArtTile& Door, const FUghRockField& Field,
		const ugh_logic* Logic)
	{
		using P = FUghCavePortal;
		const double X = Portal.X, Floor = Portal.Floor;
		if (X != Door.At.X + 16 || Floor < Door.At.Y + 12 || Floor > Door.At.Y + 36)
		{
			return TEXT("not at its door");
		}
		const int32 Column = FMath::FloorToInt32(X);
		if (!ugh_logic_solid(Logic, Column, FMath::FloorToInt32(Floor)) ||
			ugh_logic_solid(Logic, Column, FMath::FloorToInt32(Floor) - 1))
		{
			return TEXT("on no floor of the mask");
		}
		auto Rock = [&](double PX, double PY, double Depth) { return Field.Sample(FVector(PX, PY, Depth)) > 0; };
		// open from in front of the arch through its mouth, where a passenger comes out
		for (double Depth = P::Nearest; Depth <= P::Front + 16; Depth += 1)
		{
			if (Rock(X, Floor - P::Height / 2, Depth) || Rock(X, Floor - 7, Depth))
			{
				return FString::Printf(TEXT("its opening closed %.0f px deep"), Depth);
			}
		}
		if (Rock(X, Floor - 7, UghFigurePlace::DoorDepth / UnitsPerPixel))
		{
			return TEXT("no room for a passenger coming out");
		}
		// framed: rock above and beside the opening, a floor under it, the passage's end deep in
		const double Framed = P::Front + 10;
		if (!Rock(X, Floor - P::Height - P::Arch / 2, Framed) ||
			!Rock(X - P::HalfWidth - P::Arch / 2, Floor - P::Height / 2, Framed) ||
			!Rock(X + P::HalfWidth + P::Arch / 2, Floor - P::Height / 2, Framed))
		{
			return TEXT("no arch of rock around its opening");
		}
		if (!Rock(X, Floor + 1, P::Front + 5) || !Rock(X, Floor - P::Height / 2, P::End + 3))
		{
			return TEXT("no floor in its passage, or no end");
		}
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghPortalTest, "Ugh.Portals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghPortalTest::RunTest(const FString& Parameters)
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
	TSet<int32> Done;
	int32 Entrances = 0;
	for (const auto& [Players, Count] : Modes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const int32 LevelId = Simulation.GetCurrent().level_id;
			if (Done.Contains(LevelId))
			{
				continue;
			}
			Done.Add(LevelId);
			const TArray<FUghArtTile> Doors = Art.Doors(LevelId);
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(LevelId, Sprites), Doors);
			const TArray<FUghCavePortal>& Portals = Field.GetPortals();
			if (!TestEqual(FString::Printf(TEXT("level_id %d: an entrance at each door"), LevelId), Portals.Num(),
				Doors.Num()))
			{
				continue;
			}
			for (int32 Index = 0; Index < Doors.Num(); ++Index)
			{
				const FString Wrong = Problem(Portals[Index], Doors[Index], Field, Logic);
				TestTrue(FString::Printf(TEXT("level_id %d: the entrance at %.0f, %.0f: %s"), LevelId, Portals[Index].X,
					Portals[Index].Floor, *Wrong), Wrong.IsEmpty());
			}
			Entrances += Portals.Num();
		}
	}
	TestTrue(TEXT("levels with entrances"), Entrances > 0);
	AddInfo(FString::Printf(TEXT("%d levels, %d cave entrances"), Done.Num(), Entrances));
	return true;
}

#endif
