// The decorations of every level as an automation test of the editor (Ugh.Scenery): rich, standing on the rock, and
// never where they could hide a figure, a pad or its board.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghBackground.h"
#include "UghDecorations.h"
#include "UghJson.h"
#include "UghLevelArt.h"
#include "UghRockField.h"
#include "UghSimulation.h"
#include "UghSprites.h"
#include "ugh_logic.h"

namespace
{
	using FDecoration = FUghDecoration;
	using EKind = FUghDecoration::EKind;

	/** The levels of both modes: players, count. */
	constexpr int32 Modes[][2] = { { 1, 69 }, { 2, 81 } };
	/** Rich: at least this many decorations, of this many kinds, this many of them grass and flowers. */
	constexpr int32 MinCount = 150, MinKinds = 6, MinMeadow = 100;

	/** A protected box: what it is; pixels of the screen (y down), how near the plane of the play it reaches (units). */
	struct FVolume
	{
		FString Name;
		double Left, Right, Top, Bottom, Front;
	};

	/** The box of `Decoration` reaches into `Volume` (its x and y, and nearer than its Front). */
	bool Into(const FDecoration& Decoration, const FVolume& Volume)
	{
		return Decoration.Left() < Volume.Right && Volume.Left < Decoration.Right() && Decoration.Top() < Volume.Bottom &&
			Volume.Top < Decoration.Bottom() && Decoration.Front() < Volume.Front;
	}

	bool Overlap(const FDecoration& A, const FDecoration& B)
	{
		return A.Left() < B.Right() && B.Left() < A.Right() && A.Top() < B.Bottom() && B.Top() < A.Bottom() &&
			A.Front() < B.Back() && B.Front() < A.Back();
	}

	/**
	 * Where nothing may be in the level of `Logic`: the screen above the water nearer than the slab of the play
	 * (UghDecorations::SlabFront); where a copter lands on a pad, nearer than its body and the sweep of its rotor
	 * reach; the boards with the pads' numbers, nearer than they stand.
	 */
	TArray<FVolume> Protected(const ugh_logic* Logic, int32 WaterRow, const TArray<FUghArtTile>& Signs,
		const FUghSprites& Sprites)
	{
		TArray<FVolume> Volumes;
		Volumes.Add({ TEXT("the slab of the play"), 0, double(UghShapes::ScreenWidth), 0, double(WaterRow),
			UghDecorations::SlabFront });
		for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
		{
			ugh_logic_pad Pad;
			ugh_logic_get_pad(Logic, Index, &Pad);
			for (const UghDecorations::FPadRoom& Room : { UghDecorations::PadBody, UghDecorations::PadRotor })
			{
				Volumes.Add({ FString::Printf(TEXT("pad %d's landing copter"), Pad.number), Pad.left - Room.Side,
					Pad.right + 1 + Room.Side, Pad.y - Room.Top, Pad.y - Room.Bottom, Room.Front });
			}
		}
		for (const FUghArtTile& Sign : Signs)
		{
			const FIntPoint Size = Sprites.Size(Sign.Sprite);
			Volumes.Add({ TEXT("a pad's board"), double(Sign.At.X), double(Sign.At.X + Size.X), double(Sign.At.Y),
				double(Sign.At.Y + Size.Y), AUghBackground::SignDepth + 1 });
		}
		return Volumes;
	}

	/** What is wrong with `Decoration` among `All` in the level; empty when nothing. */
	FString Problem(const FDecoration& Decoration, const TArray<FDecoration>& All, const FUghRockField& Field,
		int32 WaterRow, const TArray<FVolume>& Volumes)
	{
		if (Decoration.Left() < 0 || Decoration.Right() > UghShapes::ScreenWidth || Decoration.Top() < 0 ||
			Decoration.Bottom() >= WaterRow)
		{
			return TEXT("not on the screen above the water");
		}
		for (const FVolume& Volume : Volumes)
		{
			if (Into(Decoration, Volume))
			{
				return TEXT("in front of ") + Volume.Name;
			}
		}
		// the figures' reach: anything taller than ground cover; the rotors' and wings' sweep: the tall and the hanging
		if (Decoration.Height > UghDecorations::GroundCover && Decoration.Front() < UghDecorations::FigureReach)
		{
			return TEXT("within the figures' reach");
		}
		if ((Decoration.Hangs() || Decoration.Height > UghDecorations::Middle) &&
			Decoration.Front() < UghDecorations::SweepReach)
		{
			return TEXT("within the rotors' and wings' sweep");
		}
		// on the rock: rock right under the middle of its foot (above a liana; behind one on the back wall, somewhere
		// along it), air in the middle of the front half of its box (its back may lean on the cave's back wall)
		bool bHeld = false;
		if (Decoration.Kind == EKind::Creeper)
		{
			for (double Along = 0; Along <= 1 && !bHeld; Along += 0.125)
			{
				bHeld = Field.Sample(FVector(Decoration.X, Decoration.Y + Decoration.Height * Along,
					Decoration.Back() / UghShapes::UnitsPerPixel)) > 0;
			}
		}
		else
		{
			const double Holder = Decoration.Hangs() ? Decoration.Y - 0.5 : Decoration.Y + 0.5;
			bHeld = Field.Sample(FVector(Decoration.X, Holder, Decoration.Depth / UghShapes::UnitsPerPixel)) > 0;
		}
		if (!bHeld)
		{
			return TEXT("on no rock");
		}
		const double FrontHalf = (Decoration.Front() + Decoration.Depth) / 2 / UghShapes::UnitsPerPixel;
		if (Field.Sample(FVector(Decoration.X, (Decoration.Top() + Decoration.Bottom()) / 2, FrontHalf)) >= 0)
		{
			return TEXT("in the rock");
		}
		const bool bByFire = All.ContainsByPredicate([&](const FDecoration& Other)
		{
			return &Other != &Decoration && (Other.Kind == EKind::Campfire || Decoration.Kind == EKind::Campfire) &&
				Overlap(Decoration, Other);
		});
		return bByFire ? TEXT("in a campfire") : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghSceneryTest, "Ugh.Scenery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghSceneryTest::RunTest(const FString& Parameters)
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
			const int32 WaterRow = View.water_level / UghShapes::Subpixels;
			FUghRockField Field;
			Field.Build(Logic, Art.Draw(View.level_id, Sprites));
			const TArray<FDecoration> Decorations = UghDecorations::Plan(Logic, Field, View.level_id, WaterRow);
			TestTrue(Name + TEXT(": the same decorations every time"),
				Decorations == UghDecorations::Plan(Logic, Field, View.level_id, WaterRow));
			TSet<EKind> Kinds;
			int32 Meadow = 0;
			const TArray<FVolume> Volumes =
				Protected(Logic, WaterRow, Art.Signs(View.level_id), Sprites);
			for (const FDecoration& Decoration : Decorations)
			{
				Kinds.Add(Decoration.Kind);
				Meadow += Decoration.Kind == EKind::Grass || Decoration.Kind == EKind::Flower;
				const FString Wrong = Problem(Decoration, Decorations, Field, WaterRow, Volumes);
				TestTrue(FString::Printf(TEXT("%s: the %s at %.1f, %.1f, %.0f: %s"), *Name,
					UghDecorations::Name(Decoration.Kind),
					Decoration.X, Decoration.Y, Decoration.Depth, *Wrong), Wrong.IsEmpty());
			}
			TestTrue(FString::Printf(TEXT("%s: rich (%d decorations of %d kinds, %d grass and flowers)"), *Name,
				Decorations.Num(), Kinds.Num(), Meadow),
				Decorations.Num() >= MinCount && Kinds.Num() >= MinKinds && Meadow >= MinMeadow);
			TestTrue(Name + TEXT(": a palm"), Kinds.Contains(EKind::Palm));
			Fewest = FMath::Min(Fewest, Decorations.Num());
			Total += Decorations.Num();
			++Checked;
		}
	}
	AddInfo(FString::Printf(TEXT("%d levels: %d decorations on average, at least %d"), Checked,
		Total / FMath::Max(Checked, 1), Fewest));
	return true;
}

#endif
