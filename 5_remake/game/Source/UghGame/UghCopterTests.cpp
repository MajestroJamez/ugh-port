// The copters as automation tests of the editor: the rotor's spin (Ugh.Copter.Spin), the imported models
// (Ugh.Copter.Model).
#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UghAssets.h"
#include "UghCaveman.h"
#include "UghCopterModel.h"
#include "UghRotorSpin.h"
#include "UghShapes.h"
#include "UghSimulation.h"

namespace
{
	/** The rotor's sprites of player 1 (assets/logic/ugh-data.ugd: rotor0). */
	constexpr int32 FirstSprite = 218, LastSprite = 223;
	constexpr double FrameSeconds = 1.0 / 60;

	/** Frames of `Seconds` at 60 fps over steps of the logic in which the sprite moves on every `StepsPerSprite`. */
	void Run(FUghRotorSpin& Spin, int32& Sprite, double& Time, double Seconds, int32 StepsPerSprite)
	{
		for (const double End = Time + Seconds; Time < End; Time += FrameSeconds)
		{
			const int32 Step = int32((Time + FrameSeconds) * FUghSimulation::TickRate);
			if (StepsPerSprite > 0)
			{
				Sprite = FirstSprite + (Step / StepsPerSprite) % (LastSprite - FirstSprite + 1);
			}
			Spin.Update(Sprite, FrameSeconds);
		}
	}

	/** The body's box (cm, around the copter's origin) of UGH_LOGIC_COPTER_BODY_*. */
	FBox BodyBox()
	{
		const double Half = (UghShapes::CopterBodyRight - UghShapes::CopterBodyLeft + 1) * UghShapes::UnitsPerPixel / 2;
		return FBox(FVector(-Half, -UghCopterModel::BodyHalfDepth, 0),
			FVector(Half, UghCopterModel::BodyHalfDepth, UghShapes::CopterBodyHeight * UghShapes::UnitsPerPixel));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghRotorSpinTest, "Ugh.Copter.Spin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghRotorSpinTest::RunTest(const FString& Parameters)
{
	FUghRotorSpin Spin;
	int32 Sprite = FirstSprite;
	double Time = 0;
	Run(Spin, Sprite, Time, 3, 3);
	const double Expected = FUghSimulation::TickRate / 3 / FUghRotorSpin::SpritesPerTurn;
	TestTrue(FString::Printf(TEXT("turns as fast as its sprites (%.2f, wanted %.2f a second)"), Spin.Speed(), Expected),
		FMath::Abs(Spin.Speed() - Expected) < Expected * 0.15);
	const double Turn = Spin.RotorTurn();
	Run(Spin, Sprite, Time, 0.5, 3);
	const double Off = FMath::Frac(Spin.RotorTurn() - Turn - Expected * 0.5 + 0.5) - 0.5;   // turns, -0.5 .. 0.5
	TestTrue(FString::Printf(TEXT("turns on smoothly (%.2f turns off)"), Off), FMath::Abs(Off) < 0.15);
	TestTrue(TEXT("the crank turns slower"), FMath::IsNearlyEqual(
		FMath::Frac(Spin.PedalTurn() * FUghRotorSpin::RotorTurnsPerPedal), Spin.RotorTurn(), 1e-6));
	Run(Spin, Sprite, Time, 2, 0);
	TestTrue(FString::Printf(TEXT("stops when its sprites stop (%.3f)"), Spin.Speed()), Spin.Speed() < 0.01);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghCopterModelTest, "Ugh.Copter.Model",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghCopterModelTest::RunTest(const FString& Parameters)
{
	using namespace UghCopterModel;
	FUghCaveman Caveman;
	UStaticMesh* Body = UghAssets::Mesh(UghAssets::Copter, Bodies[0]);
	if (!Body || !Caveman.Load())
	{
		AddInfo(TEXT("the copter's models are not imported (fetch-assets.ps1, build.ps1): nothing to check"));
		return true;
	}
	// the body fills the copter's body in the slab of the play and stays in it
	const FBox Wanted = BodyBox();
	for (const TCHAR* Name : Bodies)
	{
		const FBox Box = UghAssets::Mesh(UghAssets::Copter, Name)->GetBoundingBox();
		TestTrue(FString::Printf(TEXT("%s %s inside the body %s"), Name, *Box.ToString(), *Wanted.ToString()),
			Wanted.ExpandBy(FVector(1, 0.5, 1)).IsInside(Box));
		TestTrue(FString::Printf(TEXT("%s fills the body"), Name),
			Box.GetSize().X > Wanted.GetSize().X * 0.95 && Box.GetSize().Z > Wanted.GetSize().Z * 0.95);
	}
	for (const TCHAR* Name : Rotors)
	{
		const FBox Box = UghAssets::Mesh(UghAssets::Copter, Name)->GetBoundingBox().ShiftBy(RotorHub);
		TestTrue(FString::Printf(TEXT("%s at the top of the body %s"), Name, *Box.ToString()),
			Box.Min.Z > Wanted.Max.Z * 0.8 && Box.Max.Z < Wanted.Max.Z + 5);
	}
	// the stone passenger is as big as its sprite (16 x 11 px) and stands on its origin
	const FBox Stone = UghAssets::Mesh(UghAssets::StonePassenger, StonePassenger)->GetBoundingBox();
	TestTrue(FString::Printf(TEXT("the stone passenger %s"), *Stone.ToString()),
		FMath::IsNearlyEqual(Stone.GetSize().X, 16 * UghShapes::UnitsPerPixel, 15.0) &&
		FMath::IsNearlyEqual(Stone.GetSize().Z, 11 * UghShapes::UnitsPerPixel, 15.0) && FMath::Abs(Stone.Min.Z) < 5);
	const FBox Cradle = UghAssets::Mesh(UghAssets::Copter, Sling)->GetBoundingBox();
	TestTrue(TEXT("the sling holds it"),
		Cradle.Min.Z < Hanging.Z + Stone.GetSize().Z / 2 && Cradle.Max.Z <= Wanted.Min.Z + 20);
	// every look of a passenger in a cabin is a caveman but the stone passenger's (4)
	for (int32 Look = 1; Look <= 4; ++Look)
	{
		TestEqual(FString::Printf(TEXT("look %d is a person"), Look), FUghCaveman::IsPassenger(Look), Look < 4);
	}
	return true;
}

#endif
