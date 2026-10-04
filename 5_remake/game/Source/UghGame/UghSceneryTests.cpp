// The decorations of every level as an automation test of the editor (Ugh.Scenery): where they stand.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghDecorations.h"
#include "UghLedges.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };

	bool Solid(const ugh_logic* Logic, int32 X, int32 Y) { return ugh_logic_solid(Logic, X, Y) != 0; }

	/** What is wrong with where `Decoration` stands in the level of `Logic`; empty when nothing. */
	FString Problem(const ugh_logic* Logic, const FUghDecoration& Decoration, int32 WaterRow)
	{
		const int32 Left = Decoration.X - Decoration.Width / 2, Right = Left + Decoration.Width;
		if (Decoration.Height <= 0 || Decoration.Y >= WaterRow || Left < 0 || Right > UghShapes::ScreenWidth)
		{
			return TEXT("not on a dry ledge on the screen");
		}
		// a palm stands on its trunk, a rock on its whole width
		const bool bPalm = Decoration.Kind == FUghDecoration::EKind::Palm;
		for (int32 X = bPalm ? Decoration.X - 1 : Left; X < (bPalm ? Decoration.X + 1 : Right); ++X)
		{
			if (!Solid(Logic, X, Decoration.Y))
			{
				return FString::Printf(TEXT("no ground at %d"), X);
			}
		}
		for (int32 X = Left; X < Right; ++X)
		{
			if (UghLedges::RoomAbove(Logic, X, Decoration.Y, Decoration.Height) < Decoration.Height)
			{
				return FString::Printf(TEXT("no room above %d"), X);
			}
		}
		// only a palm tall enough to keep its crown above the pad's sign stands on a pad's ledge
		if (UghLedges::NearPad(Logic, Left, Right, Decoration.Y, 0) &&
			(!bPalm || Decoration.Height < UghDecorations::PalmOverPadMin))
		{
			return TEXT("on a pad");
		}
		if (Decoration.Depth - Decoration.Width * UghShapes::UnitsPerPixel / 2 < UghShapes::PlaneThickness / 2)
		{
			return TEXT("in the slab of the play");
		}
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghSceneryTest, "Ugh.Scenery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghSceneryTest::RunTest(const FString& Parameters)
{
	FUghSimulation Simulation;
	FString Error;
	const FString Data =
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets/logic/ugh-data.ugd"));
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Data, Error)))
	{
		return false;
	}
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
			const TOptional<FIntPoint> Hearth = UghLedges::FindHearth(Logic, WaterRow);
			const TArray<FUghDecoration> Decorations = UghDecorations::Plan(Logic, View.level_id, WaterRow, Hearth);
			TestTrue(Name + TEXT(": the same decorations every time"),
				Decorations == UghDecorations::Plan(Logic, View.level_id, WaterRow, Hearth));
			TestTrue(Name + TEXT(": a palm"), Decorations.ContainsByPredicate(
				[](const FUghDecoration& Decoration) { return Decoration.Kind == FUghDecoration::EKind::Palm; }));
			for (const FUghDecoration& Decoration : Decorations)
			{
				const FString Wrong = Problem(Logic, Decoration, WaterRow);
				TestTrue(FString::Printf(TEXT("%s: the decoration at %d, %d: %s"), *Name, Decoration.X, Decoration.Y,
					*Wrong), Wrong.IsEmpty());
			}
		}
	}
	return true;
}

#endif
