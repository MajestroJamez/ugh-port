// The cave's air as an automation test of the editor (Ugh.CaveAir): its map follows the collision mask.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghCaveAir.h"
#include "UghSimulation.h"
#include "ugh_logic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghCaveAirTest, "Ugh.CaveAir", EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

/**
 * Level 1 and a level of the team: no air in a cell all rock, all air in a cell without rock; a shaft only where the way
 * back against the light to the screen's edge is air (none in a cell whose next cell that way is rock); a fire glows
 * most at its own cell, less further, nothing beyond its reach nor in rock; the same map again; none before a level.
 */
bool FUghCaveAirTest::RunTest(const FString& Parameters)
{
	using namespace UghCaveAir;
	FUghSimulation Simulation;
	FString Error;
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error)))
	{
		return false;
	}
	TestTrue(TEXT("no map before a level"), Map(nullptr, FVector2D(0, 1), {}).IsEmpty());
	const FVector2D Way = FVector2D(0.35, 1).GetSafeNormal();
	for (const FUghGameChoice& Choice : { FUghGameChoice{ 1, 1, 0 }, FUghGameChoice{ 2, 1, 20 } })
	{
		Simulation.Preview(Choice);
		const ugh_logic* Logic = Simulation.GetLogic();
		auto Solid = [&](int32 I, int32 J)
		{
			int32 Count = 0;
			for (int32 Y = 0; Y < Scale; ++Y)
			{
				for (int32 X = 0; X < Scale; ++X)
				{
					Count += ugh_logic_solid(Logic, I * Scale + X, J * Scale + Y) ? 1 : 0;
				}
			}
			return Count;
		};
		// a fire in the middle of the largest stretch of air of a row
		TOptional<FIntPoint> FireCell;
		for (int32 J = Height / 3; J < Height && !FireCell; ++J)
		{
			for (int32 I = 4; I < Width - 4 && !FireCell; ++I)
			{
				bool bAir = true;
				for (int32 X = -4; X <= 4; ++X)
				{
					bAir = bAir && Solid(I + X, J) == 0 && Solid(I + X, J - 1) == 0 && Solid(I + X, J + 1) == 0;
				}
				FireCell = bAir ? FIntPoint(I, J) : TOptional<FIntPoint>();
			}
		}
		if (!TestTrue(TEXT("air for a fire"), FireCell.IsSet()))
		{
			return false;
		}
		const FUghAirFire Fire{ FVector2D((FireCell->X + 0.5) * Scale, (FireCell->Y + 0.5) * Scale), 40, 1.f };
		const TArray<FLinearColor> Cells = Map(Logic, Way, { Fire });
		if (!TestEqual(TEXT("cells"), Cells.Num(), Width * Height))
		{
			return false;
		}
		TestTrue(TEXT("the same map again"), Cells == Map(Logic, Way, { Fire }));
		int32 Wrong = 0, Shafts = 0;
		for (int32 J = 1; J < Height - 1; ++J)
		{
			for (int32 I = 1; I < Width - 1; ++I)
			{
				const FLinearColor& Cell = Cells[J * Width + I];
				// (softened by the cells around: a cell inside rock or air all around)
				int32 Around = 0;
				for (int32 Y = -1; Y <= 1; ++Y)
				{
					for (int32 X = -1; X <= 1; ++X)
					{
						Around += Solid(I + X, J + Y);
					}
				}
				const bool bRock = Around == 9 * Scale * Scale, bAir = Around == 0;
				Wrong += bRock && (Cell.A > 0.01f || Cell.B > 0.01f || Cell.G > 0.01f) ? 1 : 0;
				Wrong += bAir && Cell.A < 0.99f ? 1 : 0;
				Shafts += bAir && Cell.G > 0.1f ? 1 : 0;
			}
		}
		TestEqual(TEXT("no air, glow or shaft in rock, air in air"), Wrong, 0);
		TestTrue(TEXT("some air lit by a shaft"), Shafts > 0);
		const float AtFire = Cells[FireCell->Y * Width + FireCell->X].B;
		TestTrue(TEXT("the fire glows at its cell"), AtFire > 0.5f);
		TestTrue(TEXT("less further"), Cells[FireCell->Y * Width + FireCell->X + 4].B < AtFire);
		const int32 Far = FireCell->X + int32(Fire.Reach / Scale) + 3;
		TestTrue(TEXT("nothing beyond its reach"), Far >= Width || Cells[FireCell->Y * Width + Far].B < 0.01f);
	}
	return true;
}

#endif
