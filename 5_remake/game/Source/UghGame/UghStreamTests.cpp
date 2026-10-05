// The springs, streams and waterfalls of every level as an automation test of the editor (Ugh.Streams): where there is
// room for them, never where they could change the play or hide a figure, a pad or its board.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghDecorations.h"
#include "UghFalls.h"
#include "UghGround.h"
#include "UghJson.h"
#include "UghLedges.h"
#include "UghLevelArt.h"
#include "UghPadSigns.h"
#include "UghRockField.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "UghStreams.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };
	/** At least this many levels have a stream. */
	constexpr int32 MinLevels = 10;
	constexpr int32 Height = UghShapes::ScreenHeight;

	/** What is wrong with `Stream` in the level of `Logic` (its rock `Field`, boards `Signs`); empty when nothing. */
	FString Problem(const FUghStream& Stream, const ugh_logic* Logic, const FUghRockField& Field,
		const TArray<FUghPadSign>& Signs, int32 WaterRow)
	{
		const FUghGround Ground(Logic, Field);
		const FBox2D Area = Stream.Area();
		// the rock all through its area from the ledge down under the water: its waterfall hides no figure
		for (int32 Y = Stream.Y; Y < FMath::Min(FMath::CeilToInt32(Area.Max.Y), Height); ++Y)
		{
			for (int32 X = FMath::FloorToInt32(Area.Min.X); X < FMath::CeilToInt32(Area.Max.X); ++X)
			{
				if (!Ground.Solid(X, Y))
				{
					return FString::Printf(TEXT("pixel %d, %d under it is not rock"), X, Y);
				}
			}
		}
		if (Stream.Foot != WaterRow || Stream.Foot - Stream.Y < UghStreams::MinDrop ||
			UghLedges::RoomAbove(Logic, FMath::FloorToInt32(Stream.X()), Stream.Y, UghStreams::Room) < UghStreams::Room)
		{
			return TEXT("no ledge with room above it high enough over the water");
		}
		// away from where copters land, from the boards and the cave's entrances
		const FBox2D Around = Area.ExpandBy(1);
		for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
		{
			ugh_logic_pad Pad;
			ugh_logic_get_pad(Logic, Index, &Pad);
			for (const UghDecorations::FPadRoom& Room : { UghDecorations::PadBody, UghDecorations::PadRotor })
			{
				if (Around.Intersect(FBox2D(FVector2D(Pad.left - Room.Side, Pad.y - Room.Top),
					FVector2D(Pad.right + 1 + Room.Side, Pad.y - Room.Bottom))))
				{
					return FString::Printf(TEXT("where a copter lands on pad %d"), Pad.number);
				}
			}
		}
		if (Signs.ContainsByPredicate([&](const FUghPadSign& Sign) { return Around.Intersect(Sign.Box()); }) ||
			Field.GetPortals().ContainsByPredicate([&](const FUghCavePortal& Portal)
			{
				return Around.Intersect(Portal.Reach());
			}))
		{
			return TEXT("at a pad's board or a cave's entrance");
		}
		// the bridge: across the stream and the slab of the play, its logs under the ledge's surface, over rock
		const double Surface = UghShapes::ToWorld(0, Stream.Y, 0).Z;
		if (Stream.BridgeLeft > Stream.Left - 1 || Stream.BridgeRight < Stream.Right + 1 ||
			Stream.BridgeFront > -UghShapes::PlaneThickness / 2 || Stream.BridgeBack < UghShapes::PlaneThickness / 2)
		{
			return TEXT("its bridge does not cross it where the figures walk");
		}
		for (const FTransform& Log : UghFalls::Bridge(Stream))
		{
			const FBox Box = FBox(FVector(-UghShapes::ShapeSize / 2), FVector(UghShapes::ShapeSize / 2)).TransformBy(Log);
			const FVector Left = UghShapes::ToWorld(Area.Min.X, 0, 0), Right = UghShapes::ToWorld(Area.Max.X, 0, 0);
			if (Box.Min.X < Left.X || Box.Max.X > Right.X)
			{
				return TEXT("a log of its bridge beyond the ledge's rock");
			}
			// (world y is minus the depth)
			const bool bDeck = Box.Max.Z <= Surface - UghFalls::UnderSurface + 0.5;
			const bool bRail = -Box.Max.Y >= UghDecorations::SlabFront &&
				Box.Max.Z <= Surface + UghDecorations::GroundCover * UghShapes::UnitsPerPixel;
			if (!bDeck && !bRail)
			{
				return TEXT("a log of its bridge over the ledge's surface where the figures walk");
			}
		}
		// the waterfall in front of the face, coming nearer as it falls
		for (int32 Row = 0; Row < Stream.Fall.Num(); ++Row)
		{
			const double Y = Stream.Y + FMath::Max(Row * UghStreams::FallStep, 0.5);
			for (const double X : { Stream.Left, Stream.X(), Stream.Right })
			{
				if (Field.Sample(FVector(X, Y, Stream.Fall[Row] / UghShapes::UnitsPerPixel)) >= 0)
				{
					return FString::Printf(TEXT("its waterfall in the rock at %.1f, %.1f"), X, Y);
				}
			}
			if (Row > 0 && Stream.Fall[Row] > Stream.Fall[Row - 1])
			{
				return TEXT("its waterfall going back to the rock");
			}
		}
		if (Stream.Y + (Stream.Fall.Num() - 1) * UghStreams::FallStep < Stream.Foot)
		{
			return TEXT("its waterfall not down to the water");
		}
		// the spring: its hole in the back wall where its bed ends, the rock behind it
		const FVector Hole(Stream.X(), Stream.Y - Stream.SpringHeight - 1, Stream.Bed.Last().X / UghShapes::UnitsPerPixel);
		if (Stream.Spring - Stream.Bed.Last().X > UghShapes::UnitsPerPixel || Field.Sample(Hole + FVector(0, 0, 1.5)) <= 0)
		{
			return TEXT("its spring not on the back wall");
		}
		// the water lies over its bed (it may fill a dip), never in the rock, the floor near under it
		for (const FVector& Bed : Stream.Bed)
		{
			const double Depth = Bed.X / UghShapes::UnitsPerPixel, Water = (Bed.Y + Bed.Z) / 2;
			if (Field.Sample(FVector(Stream.X(), Water - 0.3, Depth)) >= 0 ||
				Field.Sample(FVector(Stream.X(), Water + 1.5, Depth)) <= 0)
			{
				return FString::Printf(TEXT("its bed off the floor %.0f deep"), Bed.X);
			}
		}
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghStreamsTest, "Ugh.Streams",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghStreamsTest::RunTest(const FString& Parameters)
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
	TArray<FString> With;
	for (const auto& [Players, Count] : Modes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const ugh_logic_view& View = Simulation.GetCurrent();
			const FString Name = FString::Printf(TEXT("%s-%02d"), Players == 1 ? TEXT("1p") : TEXT("team"), Level + 1);
			const int32 WaterRow = View.water_level / UghShapes::Subpixels;
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(View.level_id, Sprites), Art.Doors(View.level_id));
			const TArray<FUghPadSign> Signs = UghPadSigns::Plan(Logic, FUghGround(Logic, Field), Art.Signs(View.level_id));
			const TArray<FUghStream> Streams = UghStreams::Plan(Logic, Field, WaterRow, Signs);
			TestTrue(Name + TEXT(": the same streams every time"),
				Streams == UghStreams::Plan(Logic, Field, WaterRow, Signs));
			TestTrue(Name + TEXT(": few enough for the sea"), Streams.Num() <= UghStreams::MaxStreams);
			for (const FUghStream& Stream : Streams)
			{
				const FString Wrong = Problem(Stream, Logic, Field, Signs, WaterRow);
				TestTrue(FString::Printf(TEXT("%s: the stream at %.0f, %d: %s"), *Name, Stream.X(), Stream.Y, *Wrong),
					Wrong.IsEmpty());
			}
			if (!Streams.IsEmpty())
			{
				With.Add(FString::Printf(TEXT("%s (%d px)"), *Name, Streams[0].Foot - Streams[0].Y));
			}
		}
	}
	AddInfo(FString::Printf(TEXT("%d levels with a stream: %s"), With.Num(), *FString::Join(With, TEXT(", "))));
	TestTrue(FString::Printf(TEXT("streams in at least %d levels"), MinLevels), With.Num() >= MinLevels);
	return true;
}

#endif
