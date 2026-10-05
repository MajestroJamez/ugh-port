// The pads' boards and the speech bubbles as automation tests of the editor: every pad of every level has exactly one
// board with its number (Ugh.Signs), a bubble shows what its sprite says and points its one tail at its passenger
// (Ugh.Bubbles).
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghBubbles.h"
#include "UghGround.h"
#include "UghJson.h"
#include "UghLevelArt.h"
#include "UghPadSigns.h"
#include "UghRockField.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "ugh_logic.h"

namespace
{
	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };

	/**
	 * Where the original's drawing has another board than the pad's number: level_id 31 draws II on pad 1 as well as on
	 * pad 2 (its passengers' bubbles show pad 1 with one stroke).
	 */
	constexpr int32 DrawingSlips[][2] = { { 31, 0 } };

	/** The data, as the game reads it; false (and the test failed) when it is missing. */
	bool LoadData(FAutomationTestBase& Test, FUghSimulation& Simulation, FUghSprites& Sprites, FUghLevelArt& Art)
	{
		const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
		TSharedPtr<FJsonObject> Levels;
		FString Error;
		const FString LevelsPath = Assets / UghJson::LevelsFile;
		const bool bLoaded = Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error) &&
			Sprites.Load(Assets, Error) && UghJson::ReadObject(LevelsPath, Levels, Error) &&
			Art.Load(*Levels, LevelsPath, Error);
		return Test.TestTrue(TEXT("the game data: ") + Error, bLoaded);
	}

	bool Overlap(const FBox2D& A, const FBox2D& B)
	{
		return A.Min.X < B.Max.X && B.Min.X < A.Max.X && A.Min.Y < B.Max.Y && B.Min.Y < A.Max.Y;
	}

	bool IsSlip(int32 LevelId, int32 Pad)
	{
		for (const auto& [Id, Index] : DrawingSlips)
		{
			if (Id == LevelId && Index == Pad)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghSignsTest, "Ugh.Signs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghSignsTest::RunTest(const FString& Parameters)
{
	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghLevelArt Art;
	if (!LoadData(*this, Simulation, Sprites, Art))
	{
		return false;
	}
	int32 Boards = 0, Drawn = 0;
	TSet<int32> Slips, Twins;   // the levels (level_id) with them
	for (const auto& [Players, Count] : Modes)
	{
		for (int32 Level = 0; Level < Count; ++Level)
		{
			Simulation.Preview(FUghGameChoice{ Players, 1, Level });
			const ugh_logic* Logic = Simulation.GetLogic();
			const int32 LevelId = Simulation.GetCurrent().level_id;
			const FString Name = FString::Printf(TEXT("%s level %d"), Players == 1 ? TEXT("one player") : TEXT("team"),
				Level + 1);
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(LevelId, Sprites), Art.Doors(LevelId));
			const FUghGround Ground(Logic, Field);
			const TArray<FUghArtTile> Tiles = Art.Signs(LevelId);
			const TArray<FUghPadSign> Signs = UghPadSigns::Plan(Logic, Ground, Tiles);
			TestEqual(Name + TEXT(": a board a pad"), Signs.Num(), ugh_logic_pad_count(Logic));
			for (int32 Index = 0; Index < Signs.Num(); ++Index)
			{
				const FUghPadSign& Sign = Signs[Index];
				ugh_logic_pad Pad;
				ugh_logic_get_pad(Logic, Index, &Pad);
				const FString What = FString::Printf(TEXT("%s: pad %d (number %d)"), *Name, Index, Pad.number);
				TestEqual(What + TEXT(": its board"), Sign.Pad, Index);
				TestEqual(What + TEXT(": the number"), Sign.Number, Pad.number);
				TestEqual(What + TEXT(": the marks"), Sign.Marks, Pad.number <= UghPadSigns::MostMarks ? Pad.number : 0);
				if (Sign.Sprite)
				{
					const bool bSlip = IsSlip(LevelId, Index);
					TestEqual(What + TEXT(": the original's board says the same"),
						UghPadSigns::MarksOfSprite(*Sign.Sprite).Get(-1) == Sign.Marks, !bSlip);
					Drawn += 1;
					if (bSlip)
					{
						Slips.Add(LevelId);
					}
				}
				const FBox2D Box = Sign.Box();
				TestTrue(What + TEXT(": on the screen, over its pad"), Box.Min.X >= 0 &&
					Box.Max.X <= UghShapes::ScreenWidth && Box.Min.Y >= 0 && Sign.X >= Pad.left - UghPadSigns::Width &&
					Sign.X <= Pad.right + UghPadSigns::Width);
				const TOptional<double> Floor = Ground.Floor(Sign.X, Pad.y, UghPadSigns::Depth);
				TestTrue(What + TEXT(": stands on the rock"), Floor.IsSet() && Sign.Y == *Floor);
				for (int32 Other = 0; Other < Index; ++Other)
				{
					TestFalse(What + FString::Printf(TEXT(": clear of pad %d's board"), Other),
						Overlap(Signs[Other].Box(), Box));
				}
			}
			// every board the original draws stands where it draws it, but a second one on a pad (its twin stands)
			for (const FUghArtTile& Tile : Tiles)
			{
				const bool bShown = Signs.ContainsByPredicate([&](const FUghPadSign& Sign)
				{
					return Sign.Sprite == Tile.Sprite && Sign.X == Tile.At.X + UghPadSigns::PostX;
				});
				const bool bTwin = !bShown && Signs.ContainsByPredicate([&](const FUghPadSign& Sign)
				{
					return Sign.Sprite == Tile.Sprite && Overlap(Sign.Box(), FBox2D(FVector2D(0, Tile.At.Y),
						FVector2D(UghShapes::ScreenWidth, Tile.At.Y + UghPadSigns::Height)));
				});
				TestTrue(FString::Printf(TEXT("%s: the original's board at %d, %d"), *Name, Tile.At.X, Tile.At.Y),
					bShown || bTwin);
				if (bTwin)
				{
					Twins.Add(LevelId);
				}
			}
			Boards += Signs.Num();
		}
	}
	TestEqual(TEXT("the original's slips"), Slips.Num(), int32(UE_ARRAY_COUNT(DrawingSlips)));
	TestTrue(TEXT("the original's second board on a pad: II twice on pad 2 of level_id 74"),
		Twins.Num() == 1 && Twins.Contains(74));
	static_assert(UghPadSigns::Front >= UghDecorations::FigureReach);
	AddInfo(FString::Printf(TEXT("%d boards, %d of them where the original draws them"), Boards, Drawn));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghBubblesTest, "Ugh.Bubbles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghBubblesTest::RunTest(const FString& Parameters)
{
	FUghSimulation Simulation;
	FUghSprites Sprites;
	FUghLevelArt Art;
	if (!LoadData(*this, Simulation, Sprites, Art))
	{
		return false;
	}
	// what each bubble of the data shows: the boards of pads 1 .. 5, a blank one, a question
	TArray<FUghBubbleLook> Looks;
	for (int32 Sprite = 0; Sprite < Sprites.Count(); ++Sprite)
	{
		if (const TOptional<FUghBubbleLook> Look = UghBubbles::Look(Simulation.GetLogic(), Sprite))
		{
			Looks.Add(*Look);
		}
	}
	const TArray<FUghBubbleLook> Expected = { { 1, false }, { 2, false }, { 3, false }, { 4, false }, { 5, false },
		{ 0, false }, { 0, true } };
	TestTrue(TEXT("the bubbles show the pads 1 .. 5, a blank board, a question"), Looks == Expected);
	TestFalse(TEXT("a passenger's sprite is no bubble"), UghBubbles::Look(Simulation.GetLogic(), 395).IsSet());

	// one tail, on the left as the original's where the bubble fits on the screen right of its passenger, else on the
	// right: its tip at its passenger's head
	constexpr double PassengerWidth = 16, Top = 100;
	for (double X = 0; X <= UghShapes::ScreenWidth - PassengerWidth; X += 0.5)
	{
		const FUghBubblePlace Place = UghBubbles::Place(FVector2D(X, Top), PassengerWidth);
		const bool bFitsRight = X + UghBubbles::OffsetX + UghBubbles::Width <= UghShapes::ScreenWidth;
		const FString What = FString::Printf(TEXT("a passenger at %.1f"), X);
		TestEqual(What + TEXT(": the tail on the left where the original has it"), Place.bTailLeft, bFitsRight);
		TestTrue(What + TEXT(": the bubble on the screen"), Place.At.X >= 0 &&
			Place.At.X + UghBubbles::Width <= UghShapes::ScreenWidth);
		TestTrue(What + TEXT(": above the passenger"), FMath::IsNearlyEqual(Place.At.Y, Top + UghBubbles::OffsetY));
		if (bFitsRight)
		{
			TestTrue(What + TEXT(": where the original draws it"), Place.At.X == X + UghBubbles::OffsetX);
		}
		const FVector2D Tip = Place.Tip();
		TestTrue(What + TEXT(": the tail points at the passenger's head"), Tip.X > X && Tip.X < X + PassengerWidth &&
			Tip.Y >= Top && Tip.Y <= Top + 3);
		TestTrue(What + TEXT(": the tail at the side towards the passenger"),
			(Tip.X < Place.At.X + UghBubbles::Width / 2) == Place.bTailLeft);
	}

	// the picture: opaque in the middle, the tail's tip on its side only, clear at the other bottom corner
	for (const bool bLeft : { true, false })
	{
		const TArray<FColor> Picture = UghBubbles::Draw({ 3, false }, bLeft);
		auto At = [&](double X, double Y)
		{
			return Picture[FMath::FloorToInt32(Y * UghBubbles::Scale) * UghBubbles::PictureWidth +
				FMath::FloorToInt32(X * UghBubbles::Scale)].A;
		};
		const double TipX = bLeft ? 1 : UghBubbles::Width - 1, OtherX = UghBubbles::Width - TipX;
		const FString Side = bLeft ? TEXT("left") : TEXT("right");
		TestEqual(TEXT("the picture's size"), Picture.Num(), UghBubbles::PictureWidth * UghBubbles::PictureHeight);
		TestTrue(TEXT("the bubble is opaque in the middle"), At(8, 6) == 255);
		TestTrue(TEXT("the tail on the ") + Side, At(TipX, 14.2) > 128);
		TestTrue(TEXT("no tail but on the ") + Side, At(OtherX, 14.2) == 0 && At(OtherX, 13.2) == 0);
	}
	return true;
}

#endif
