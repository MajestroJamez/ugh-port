// The motion blur of the play by the copters' speed, as an automation test of the editor (Ugh.MotionBlur).
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UghMotionBlur.h"
#include "UghShapes.h"

namespace
{
	/** A view of the play with one copter at x, y (pixels). */
	ugh_logic_view Play(double X, double Y)
	{
		ugh_logic_view View{};
		View.phase = UGH_LOGIC_PHASE_PLAY;
		View.level_id = 0;
		View.copter_count = 1;
		View.copters[0].x = FMath::RoundToInt32(X * UghShapes::Subpixels);
		View.copters[0].y = FMath::RoundToInt32(Y * UghShapes::Subpixels);
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghMotionBlurTest, "Ugh.MotionBlur",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghMotionBlurTest::RunTest(const FString& Parameters)
{
	using FBlur = FUghMotionBlur;
	// a hovering or slow copter rests (its rotor turning), a fast one blurs fully, more the faster in between
	TestEqual(TEXT("still"), FBlur::AmountAt(0), FBlur::Rest);
	TestEqual(TEXT("slow"), FBlur::AmountAt(FBlur::SlowPixels), FBlur::Rest);
	TestEqual(TEXT("fast"), FBlur::AmountAt(FBlur::FastPixels), FBlur::Full);
	TestEqual(TEXT("top speed"), FBlur::AmountAt(3), FBlur::Full);
	double Last = 0;
	for (double Speed = 0; Speed <= 3; Speed += 0.05)
	{
		const double Amount = FBlur::AmountAt(Speed);
		TestTrue(FString::Printf(TEXT("more blur the faster (%.2f px a step)"), Speed), Amount >= Last);
		Last = Amount;
	}
	TestTrue(TEXT("blur within the engine's range"), FBlur::Rest > 0 && FBlur::Full <= 1);

	// the speed of the fastest copter from the views of the last two steps; none outside the play; a jump none
	TestEqual(TEXT("straight"), FBlur::Speed(Play(100, 50), Play(103, 50)), 3.0, 1e-6);
	TestEqual(TEXT("diagonally"), FBlur::Speed(Play(100, 50), Play(102, 52)), FMath::Sqrt(8.0), 1e-6);
	ugh_logic_view Team = Play(100, 50), TeamNext = Play(100.5, 50);
	Team.copter_count = TeamNext.copter_count = 2;
	TeamNext.copters[1] = Team.copters[1] = Team.copters[0];
	TeamNext.copters[1].y += 2 * UghShapes::Subpixels;
	TestEqual(TEXT("the faster of two"), FBlur::Speed(Team, TeamNext), 2.0, 1e-6);
	ugh_logic_view Caption = Play(100, 50);
	Caption.phase = UGH_LOGIC_PHASE_CAPTION;
	TestEqual(TEXT("not in the play"), FBlur::Speed(Play(100, 50), Caption), 0.0);
	TestTrue(TEXT("a jump"), FBlur::Speed(Play(100, 50), Play(200, 120)) < 0);

	// it follows the speed smoothly, at once none in the frame a copter jumped
	FUghMotionBlur Blur;
	TestEqual(TEXT("at first at rest"), double(Blur.GetAmount()), FBlur::Rest, 1e-6);
	double Previous = Blur.GetAmount();
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Blur.Update(Play(100, 50), Play(103, 50), 1.0 / 60);
		TestTrue(TEXT("rising smoothly"), Blur.GetAmount() >= Previous && Blur.GetAmount() - Previous < 0.2);
		Previous = Blur.GetAmount();
	}
	TestEqual(TEXT("full when fast a while"), double(Blur.GetAmount()), FBlur::Full, 0.01);
	Blur.Update(Play(100, 50), Play(200, 120), 1.0 / 60);
	TestEqual(TEXT("none as it jumps"), double(Blur.GetAmount()), 0.0);
	Blur.Update(Play(200, 120), Play(200, 120), 1.0 / 60);
	TestEqual(TEXT("at rest after it"), double(Blur.GetAmount()), FBlur::Rest, 1e-6);
	return true;
}

#endif
