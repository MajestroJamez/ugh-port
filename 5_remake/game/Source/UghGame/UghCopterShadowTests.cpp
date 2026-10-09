// The copters' shadows as an automation test of the editor (Ugh.CopterShadow): on the first surface below a copter
// by the collision mask, fading with its height.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UghCopterShadows.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "ugh_logic.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghCopterShadowTest, "Ugh.CopterShadow", EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter)

/**
 * Level 1 and a level of the team: a copter over a pad casts its shadow on the pad's surface, darkest standing on it,
 * fainter and wider the higher, none from FadeHeight up; anywhere above the water the shadow lies on the first solid
 * pixel under the copter's middle (air from its bottom down to there, rock there), none when that is under the water;
 * its decal reaches from under that surface to above it, under the copter's middle.
 */
bool FUghCopterShadowTest::RunTest(const FString& Parameters)
{
	FUghSimulation Simulation;
	FString Error;
	const FString Assets = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets"));
	if (!TestTrue(TEXT("the game data: ") + Error, Simulation.Load(Assets / TEXT("logic/ugh-data.ugd"), Error)))
	{
		return false;
	}
	int32 Checked = 0;
	for (const FUghGameChoice& Choice : { FUghGameChoice{ 1, 1, 0 }, FUghGameChoice{ 2, 1, 20 } })
	{
		Simulation.Preview(Choice);
		const ugh_logic* Logic = Simulation.GetLogic();
		const double WaterRow = double(Simulation.GetCurrent().water_level) / UghShapes::Subpixels;
		ugh_logic_pad Pad;
		if (!TestTrue(TEXT("a pad"), ugh_logic_get_pad(Logic, 0, &Pad) != 0))
		{
			return false;
		}
		// over the pad's middle, its bottom `Above` pixels over it
		auto Over = [&](double Above)
		{
			const double X = (Pad.left + Pad.right + 1) / 2.0 - (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;
			return UghCopterShadow::BodyAt(FVector2D(X, Pad.y - UghShapes::CopterBodyHeight - Above));
		};
		double Last = UghCopterShadow::Darkest + 1, LastWidth = 0;
		// nothing but air under the copter's body down to the pad (no ledge above the pad there)
		auto Clear = [&](double Above)
		{
			const FBox2D Body = Over(Above);
			for (int32 Row = FMath::FloorToInt32(Body.Max.Y); Row < Pad.y; ++Row)
			{
				for (int32 Column = FMath::FloorToInt32(Body.Min.X); Column < FMath::CeilToInt32(Body.Max.X); ++Column)
				{
					if (ugh_logic_solid(Logic, Column, Row))
					{
						return false;
					}
				}
			}
			return true;
		};
		for (const double Above : { 0.0, 3.0, 12.0, 30.0, 50.0, 63.0 })
		{
			if (!Clear(Above))
			{
				break;
			}
			const TOptional<FUghCopterShadow> Shadow = UghCopterShadow::Under(Logic, Over(Above), WaterRow);
			if (!TestTrue(FString::Printf(TEXT("a shadow %.0f px over pad %d"), Above, Pad.number), Shadow.IsSet()))
			{
				continue;
			}
			TestEqual(TEXT("on the pad's surface"), Shadow->Surface, double(Pad.y));
			TestEqual(TEXT("its height"), Shadow->Height, Above);
			TestTrue(TEXT("fainter and wider the higher"), Shadow->Opacity < Last && Shadow->HalfWidth > LastWidth);
			Last = Shadow->Opacity;
			LastWidth = Shadow->HalfWidth;
			const FTransform Decal = UghCopterShadow::Place(*Shadow);
			const FVector Surface = UghShapes::ToWorld(Shadow->X, Shadow->Surface, 0);
			TestTrue(TEXT("the decal around the surface under the copter"),
				FMath::IsNearlyEqual(Decal.GetLocation().X, Surface.X) &&
				Decal.GetLocation().Z - Decal.GetScale3D().X < Surface.Z &&
				Decal.GetLocation().Z + Decal.GetScale3D().X > Surface.Z &&
				Decal.GetRotation().GetForwardVector().Equals(-FVector::ZAxisVector, 1e-6));
		}
		TestEqual(TEXT("darkest standing on it"),
			UghCopterShadow::Under(Logic, Over(0), WaterRow)->Opacity, UghCopterShadow::Darkest);
		// (unless on rock nearer under it)
		const TOptional<FUghCopterShadow> High = UghCopterShadow::Under(Logic, Over(UghCopterShadow::FadeHeight), WaterRow);
		TestTrue(TEXT("none on the pad high above it"), !High || (High->Surface < Pad.y && High->Height < UghCopterShadow::FadeHeight));
		// anywhere: the first solid pixel under the copter's middle
		for (int32 Y = 0; Y + UghShapes::CopterBodyHeight < WaterRow; Y += 7)
		{
			for (int32 X = -UghShapes::CopterBodyLeft; X + UghShapes::CopterBodyRight < UghShapes::ScreenWidth; X += 5)
			{
				const FBox2D Body = UghCopterShadow::BodyAt(FVector2D(X, Y + 0.5));
				const int32 Middle = FMath::FloorToInt32(Body.GetCenter().X);
				int32 First = FMath::FloorToInt32(Body.Max.Y);
				while (First < UghShapes::ScreenHeight && !ugh_logic_solid(Logic, Middle, First))
				{
					++First;
				}
				const TOptional<FUghCopterShadow> Shadow = UghCopterShadow::Under(Logic, Body, WaterRow);
				const bool bExpected = First < UghShapes::ScreenHeight && First <= WaterRow &&
					First - Body.Max.Y < UghCopterShadow::FadeHeight;
				if (bExpected && Shadow && Shadow->Surface == First)
				{
					++Checked;
					continue;
				}
				// the middle column's surface is it, unless a column beside it under the body is higher
				const bool bHigherBeside = Shadow && Shadow->Surface < First &&
					ugh_logic_solid(Logic, Middle, int32(Shadow->Surface)) == 0;
				TestTrue(FString::Printf(TEXT("level %d: the copter at %d, %d: its shadow on the first surface below "
					"(%d, the shadow's %.0f)"), Choice.FirstLevel + 1, X, Y, First, Shadow ? Shadow->Surface : -1.0),
					bHigherBeside || (!bExpected && !Shadow));
			}
		}
	}
	TestTrue(FString::Printf(TEXT("shadows checked (%d)"), Checked), Checked > 100);
	return true;
}

#endif
