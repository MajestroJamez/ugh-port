// The scanned rock dressing the cave of every level as an automation test of the editor (Ugh.Dressing): on the back
// wall, behind every figure's sweep and the decorations in front of it, none over a spring.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghDecorations.h"
#include "UghGround.h"
#include "UghJson.h"
#include "UghLevelArt.h"
#include "UghPadSigns.h"
#include "UghRockDressing.h"
#include "UghRockField.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghStreams.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };
	/** Rich: at least this many cliffs in every level. */
	constexpr int32 MinCliffs = 10;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghDressingTest, "Ugh.Dressing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghDressingTest::RunTest(const FString& Parameters)
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
	int32 Fewest = MAX_int32, Total = 0, Checked = 0;
	for (const auto& [Players, Count] : Modes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const ugh_logic_view& View = Simulation.GetCurrent();
			const FString Name =
				FString::Printf(TEXT("%s level %d"), Players == 1 ? TEXT("one player") : TEXT("team"), Level + 1);
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(View.level_id, Sprites), Art.Doors(View.level_id));
			const FUghGround Ground(Logic, Field);
			const int32 WaterRow = View.water_level / UghShapes::Subpixels;
			const TArray<FUghPadSign> Signs = UghPadSigns::Plan(Logic, Ground, Art.Signs(View.level_id));
			const TArray<FUghStream> Streams = UghStreams::Plan(Logic, Field, WaterRow, Signs);
			Field.CarveChannels(Streams);
			const TArray<FUghDecoration> Decorations =
				UghDecorations::Plan(Logic, Field, View.level_id, WaterRow, Signs, Streams);
			const TArray<FUghRockPiece> Pieces = UghRockDressing::Plan(Logic, Field, View.level_id, Decorations, Streams);
			TestTrue(Name + TEXT(": the same pieces every time"),
				Pieces == UghRockDressing::Plan(Logic, Field, View.level_id, Decorations, Streams));
			int32 Cliffs = 0;
			for (const FUghRockPiece& Piece : Pieces)
			{
				const FString What = FString::Printf(TEXT("%s: the piece at %.1f, %.1f (radius %.1f)"), *Name, Piece.X,
					Piece.Y, Piece.Radius);
				Cliffs += Piece.Kind == FUghRockPiece::EKind::Cliff;
				TestTrue(What + TEXT(": comes out of the wall"), Piece.Out > 0 && Piece.Out <= 1 &&
					Piece.Nearest() < Piece.Surface);
				TestTrue(What + TEXT(": behind every figure's sweep"), Piece.Nearest() >= UghRockDressing::BackFront &&
					UghRockDressing::BackFront > UghDecorations::SweepReach);
				TestFalse(What + TEXT(": seen in a cave's entrance"),
					Ground.AtEntrance(UghRockDressing::ScreenBox(Piece)));
				const FUghDecoration* Hidden = Decorations.FindByPredicate([&](const FUghDecoration& Decoration)
				{
					return UghRockDressing::InFrontOf(Decoration, Piece) && Piece.Nearest() < Decoration.Depth;
				});
				TestTrue(What + TEXT(": behind the middles of the decorations in front of it"), Hidden == nullptr);
				TestFalse(What + TEXT(": over a spring"), Streams.ContainsByPredicate([&](const FUghStream& Stream)
				{
					return UghRockDressing::ScreenBox(Piece).Intersect(Stream.Course());
				}));
			}
			TestTrue(FString::Printf(TEXT("%s: rich (%d cliffs)"), *Name, Cliffs), Cliffs >= MinCliffs);
			Fewest = FMath::Min(Fewest, Cliffs);
			Total += Pieces.Num();
			++Checked;
		}
	}
	AddInfo(FString::Printf(TEXT("%d levels: %d cliffs and roots on average, at least %d cliffs"), Checked,
		Total / FMath::Max(Checked, 1), Fewest));
	return true;
}

#endif
