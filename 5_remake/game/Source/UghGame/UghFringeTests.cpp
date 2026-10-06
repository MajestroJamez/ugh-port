// The soft edges of the level as an automation test of the editor (Ugh.Fringe): the plants at the stone's edges are
// off the screen of the play, a copter at each of its limits runs into them, they bend away from it, hide it in the
// overhang and leave it alone elsewhere.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UghFringe.h"
#include "UghStage.h"

namespace
{
	using EKind = FUghFringePlant::EKind;
	using ESide = FUghFringePlant::ESide;

	/** The aspects of a window the game is played in. */
	constexpr double Aspects[] = { 16.0 / 9, 16.0 / 10 };
	/**
	 * Pixels: a copter beside the level is looked at from its top limit down to here (above the water of the lowest
	 * levels); at the top limit its body (CopterBodyLeft .. Right, from its top this far down) is hidden at least this
	 * much (a share) by plants in front of its rotor (this deep, pixels).
	 */
	constexpr double LowestSide = 160, HiddenDown = 12, Hidden = 0.7, RotorDepth = -13;

	/** Where the play camera sees a point (pixels x, y and depth units) on the plane of the play. */
	FVector2D OnPlane(const FUghCameraPose& Camera, double X, double Y, double Depth)
	{
		const FVector Eye(Camera.Location.X / UghShapes::UnitsPerPixel,
			UghShapes::ScreenHeight - Camera.Location.Z / UghShapes::UnitsPerPixel, -Camera.Location.Y);
		const FVector At(X, Y, Depth);
		const FVector Hit = Eye + (At - Eye) * (-Eye.Z / (At.Z - Eye.Z));
		return FVector2D(Hit.X, Hit.Y);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghFringeTest, "Ugh.Fringe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghFringeTest::RunTest(const FString& Parameters)
{
	const TArray<FUghFringePlant> Plants = UghFringe::Plan();
	TestTrue(FString::Printf(TEXT("%d plants"), Plants.Num()), Plants.Num() > 200);
	TestTrue(TEXT("the same plants every time"), UghFringe::Plan() == Plants);
	for (int32 Index = 0; Index < Plants.Num(); ++Index)
	{
		const int32 Above = Plants[Index].Above;
		TestTrue(TEXT("a curtain's piece after the one it hangs from"), Above < Index);
	}

	// off the screen of the play, as the play's camera sees them
	for (const double Aspect : Aspects)
	{
		const FUghCameraPose Camera = AUghStage::Play(Aspect);
		int32 Over = 0;
		for (const FUghFringePlant& Plant : Plants)
		{
			const FBox2D Box = Plant.Box();
			FBox2D Seen(ForceInit);
			for (const double Depth : { Plant.Depth, Plant.Tip().Z })
			{
				Seen += OnPlane(Camera, Box.Min.X, Box.Min.Y, Depth);
				Seen += OnPlane(Camera, Box.Max.X, Box.Max.Y, Depth);
			}
			const bool bOver = Seen.Max.X > 0 && Seen.Min.X < UghShapes::ScreenWidth && Seen.Max.Y > 0 &&
				Seen.Min.Y < UghShapes::ScreenHeight;
			if (bOver && Over++ < 5)
			{
				AddInfo(FString::Printf(TEXT("a plant (kind %d) over the screen: %s"), int32(Plant.Kind), *Seen.ToString()));
			}
		}
		TestEqual(FString::Printf(TEXT("aspect %.2f: no plant over the screen"), Aspect), Over, 0);
	}

	// a copter at its limits runs into them all along the edges, and they bend away from it
	auto Pushed = [&](const FVector2D& At, ESide Side, bool& bAway)
	{
		int32 Count = 0;
		for (const FUghFringePlant& Plant : Plants)
		{
			const FVector2D Bend = UghFringe::Bend(Plant, UghFringe::Reach(At));
			if (Bend.IsNearlyZero())
			{
				continue;
			}
			++Count;
			bAway &= Plant.Side == ESide::Left ? Bend.X < 0 : Plant.Side == ESide::Right ? Bend.X > 0 : Bend.Y > 0;
			bAway &= Plant.Side == Side || Plant.Side == ESide::Top;   // (the overhang in the corners)
		}
		return Count;
	};
	int32 Missed = 0;
	bool bAway = true;
	for (double Y = UghFringe::TopEdge; Y <= LowestSide; Y += 4)
	{
		for (const ESide Side : { ESide::Left, ESide::Right })
		{
			const double X = Side == ESide::Left ? UghFringe::LeftEdge : UghFringe::RightEdge;
			if (Pushed(FVector2D(X, Y), Side, bAway) == 0 && Missed++ < 5)
			{
				AddInfo(FString::Printf(TEXT("nothing bends at %.0f, %.0f"), X, Y));
			}
		}
	}
	for (double X = UghFringe::LeftEdge + 20; X <= UghFringe::RightEdge - 20; X += 4)
	{
		if (Pushed(FVector2D(X, UghFringe::TopEdge), ESide::Top, bAway) == 0 && Missed++ < 5)
		{
			AddInfo(FString::Printf(TEXT("nothing bends over %.0f"), X));
		}
	}
	TestEqual(TEXT("a copter at its limits runs into plants"), Missed, 0);
	TestTrue(TEXT("they bend away from it (beside it outwards, above it towards the camera)"), bAway);

	// the overhang hides a copter at the top limit
	int32 Points = 0, Covered = 0;
	for (double X = 30; X <= 280; X += 50)
	{
		for (double Across = UghShapes::CopterBodyLeft; Across <= UghShapes::CopterBodyRight; Across += 2)
		{
			for (double Down = 1; Down <= HiddenDown; Down += 2)
			{
				const FVector2D Point(X + Across, UghFringe::TopEdge + Down);
				++Points;
				Covered += Plants.ContainsByPredicate([&](const FUghFringePlant& Plant)
				{
					return Plant.Side == ESide::Top && FMath::Max(Plant.Depth, Plant.Tip().Z) <
						RotorDepth * UghShapes::UnitsPerPixel && Plant.Box().IsInside(Point);
				});
			}
		}
	}
	TestTrue(FString::Printf(TEXT("a copter at the top in the overhang: %d of %d points of it hidden"), Covered, Points),
		Covered >= Hidden * Points);

	// and away from the edges nothing moves
	int32 Moved = 0;
	for (double X = 20; X <= 270; X += 25)
	{
		for (double Y = 10; Y <= 160; Y += 25)
		{
			for (const FUghFringePlant& Plant : Plants)
			{
				Moved += UghFringe::Touch(Plant, UghFringe::Reach(FVector2D(X, Y))) > 0;
			}
		}
	}
	TestEqual(TEXT("nothing bends for a copter away from the edges"), Moved, 0);
	return true;
}

#endif
