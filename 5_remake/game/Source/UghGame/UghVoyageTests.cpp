// The flight between two levels as automation tests of the editor (Ugh.Voyage): from a level done and from the level
// selection to the next level's game's camera, through its mist, smoothly, clear of the sea and the stones.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UghHighScores.h"
#include "UghIsles.h"
#include "UghMenuView.h"
#include "UghShapes.h"
#include "UghStage.h"
#include "UghVoyage.h"

namespace
{
	constexpr double Frame = 1.0 / 120;
	/** Units: above the sea at least, clear of a stone of the archipelago at least (beside it, below its top). */
	constexpr double AboveSea = 100, Clear = 1500;
	/**
	 * No jolt (a frame of 1/120 s, outside the mist's hold): its acceleration changes at most by MaxJolt (units/s2: its
	 * speed by 1 % in a frame at most, unseen - the way's length is a table, its turns sharper than FUghIntro's), it turns
	 * at most MaxTurn degrees a frame, it ends with hardly any speed (units/s in its last frame).
	 */
	constexpr double MaxJolt = 15000, MaxTurn = 1.5, StopSpeed = 200;
	/** Frames the world takes to switch in the mist (the level built a step a frame). */
	constexpr int32 SwitchFrames = 6;

	struct FRun
	{
		double Took = 0;          // seconds to its end
		double Jolt = 0, Turn = 0, Lowest = 1e9, Nearest = 1e9;
		double JoltAt = 0, TurnAt = 0;   // when (seconds)
		bool bMistAtSwitch = false, bStartsThere = false, bEndsThere = false;
		double EndSpeed = 0, MistAtEnd = 1;
	};

	/** A flight from `From` to the stone `Shift` away, run frame by frame (hurried at `HurryAt` seconds, < 0 not). */
	FRun Fly(const FUghCameraPose& From, const FVector& Shift, bool bFromPlay, const TArray<FUghIslePlace>& Isles,
		const FVector& IslesBefore, const FVector& IslesAfter, double HurryAt = -1)
	{
		const FUghCameraPose Game = AUghStage::Play(16.0 / 9);
		const double SeaZ = UghShapes::ToWorld(0, 182, 0).Z;
		const FVector Home = UghMenuView::StoneMiddle(), Stone(Home.X, Home.Y, 0);
		FUghVoyage Voyage;
		Voyage.Start(From, Game, Shift, Stone, bFromPlay, SeaZ);
		FRun Run;
		const FUghCameraPose First = Voyage.Pose(Game, SeaZ);
		Run.bStartsThere = First.Location.Equals(From.Location, 1) && First.Rotation.Equals(From.Rotation, 0.1);
		TArray<FVector> Places;
		FRotator Last = First.Rotation;
		int32 Holding = 0, SinceSwitch = 1000;
		while (Voyage.IsFlying() && Run.Took < 30)
		{
			if (HurryAt >= 0 && Run.Took >= HurryAt)
			{
				Voyage.Hurry();
				HurryAt = -1;
			}
			if (Voyage.WantsSwitch() && ++Holding >= SwitchFrames)
			{
				Run.bMistAtSwitch = Voyage.GetMist() >= 1;
				Voyage.Switched();
				SinceSwitch = 0;
				Places.Reset();
			}
			Voyage.Advance(Frame);
			Run.Took += Frame;
			++SinceSwitch;
			const FUghCameraPose Pose = Voyage.Pose(Game, SeaZ);
			Run.Lowest = FMath::Min(Run.Lowest, Pose.Location.Z - SeaZ);
			// clear of the archipelago's stones (as they stand around the stone then)
			const FVector Offset = Voyage.IsSwitched() ? IslesAfter : IslesBefore;
			for (const FUghIslePlace& Isle : Isles)
			{
				const FVector Foot = Isle.Foot + Offset;
				if (Pose.Location.Z < FUghIsles::Top(Isle).Z + Clear && !Foot.Equals(Stone, 1))
				{
					Run.Nearest = FMath::Min(Run.Nearest,
						FVector::Dist2D(Pose.Location, Foot) - FUghIsles::StoneRadius * Isle.Scale);
				}
			}
			// (not in the thick mist: it holds still there until the world has switched, its camera then jumps by Shift)
			if (Voyage.GetMist() >= 0.99)
			{
				Places.Reset();
			}
			else if (SinceSwitch > 2)
			{
				Places.Add(Pose.Location);
				if (Places.Num() >= 4)
				{
					const int32 N = Places.Num();
					const FVector A1 = (Places[N - 1] - 2 * Places[N - 2] + Places[N - 3]) / (Frame * Frame);
					const FVector A0 = (Places[N - 2] - 2 * Places[N - 3] + Places[N - 4]) / (Frame * Frame);
					if ((A1 - A0).Size() > Run.Jolt)
					{
						Run.Jolt = (A1 - A0).Size();
						Run.JoltAt = Voyage.GetTime();
					}
				}
				const double Turned = FMath::RadiansToDegrees(Pose.Rotation.Quaternion().AngularDistance(Last.Quaternion()));
				if (Turned > Run.Turn)
				{
					Run.Turn = Turned;
					Run.TurnAt = Voyage.GetTime();
				}
			}
			Last = Pose.Rotation;
			if (Places.Num() >= 2)
			{
				Run.EndSpeed = (Places.Last() - Places[Places.Num() - 2]).Size() / Frame;
			}
		}
		const FUghCameraPose End = Voyage.Pose(Game, SeaZ);
		Run.bEndsThere = Voyage.HasArrived() && End.Location.Equals(Game.Location, 0.01) &&
			End.Rotation.Equals(Game.Rotation, 0.001) && FMath::IsNearlyEqual(End.FieldOfView, Game.FieldOfView, 0.01f);
		Run.MistAtEnd = Voyage.GetMist();
		return Run;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghVoyageTest, "Ugh.Voyage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghVoyageTest::RunTest(const FString& Parameters)
{
	const FUghCameraPose Game = AUghStage::Play(16.0 / 9);
	const FVector Home = UghMenuView::StoneMiddle(), Stone(Home.X, Home.Y, 0);
	const TArray<FUghIslePlace> Isles = FUghIsles::Layout(69, Home);
	// from a level done to the next: along a row, at a row's end (the next row), a row the other way
	for (const int32 Left : { 0, 8, 12, 30, 67 })
	{
		const FVector Before = Stone - Isles[Left].Foot, After = Stone - Isles[Left + 1].Foot;
		FVector Shift = Before - After;
		Shift.Z = 0;
		const FRun Run = Fly(Game, Shift, true, Isles, Before, After);
		const FString Name = FString::Printf(TEXT("level %d to %d"), Left + 1, Left + 2);
		TestTrue(Name + TEXT(": starts at the game's camera"), Run.bStartsThere);
		TestTrue(Name + TEXT(": ends exactly in the game's camera, the mist gone"), Run.bEndsThere && Run.MistAtEnd <= 0);
		TestTrue(Name + TEXT(": the world switches in a thick mist"), Run.bMistAtSwitch);
		TestTrue(FString::Printf(TEXT("%s: as long as it is (%.2f s)"), *Name, Run.Took),
			Run.Took <= FUghVoyage::FromPlay + SwitchFrames * Frame + 0.05);
		TestTrue(FString::Printf(TEXT("%s: above the sea (%.0f)"), *Name, Run.Lowest), Run.Lowest >= AboveSea);
		TestTrue(FString::Printf(TEXT("%s: clear of the stones (%.0f)"), *Name, Run.Nearest), Run.Nearest >= Clear);
		TestTrue(FString::Printf(TEXT("%s: no jolt (%.0f units/s2 a frame at %.2f s)"), *Name, Run.Jolt, Run.JoltAt), Run.Jolt <= MaxJolt);
		TestTrue(FString::Printf(TEXT("%s: turns smoothly (%.2f degrees a frame at %.2f s)"), *Name, Run.Turn, Run.TurnAt), Run.Turn <= MaxTurn);
		TestTrue(FString::Printf(TEXT("%s: stops softly (%.0f)"), *Name, Run.EndSpeed), Run.EndSpeed <= StopSpeed);
	}
	// from the level selection's camera choosing a stone
	FUghIsles Selection;
	Selection.Open(1, 69, FUghHighScores(), INDEX_NONE, Game, Home, UghShapes::ToWorld(0, 182, 0).Z, false);
	for (const int32 Chosen : { 0, 11, 40, 68 })
	{
		const FVector After = Stone - Isles[Chosen].Foot;
		FVector Shift = -After;
		Shift.Z = 0;
		const FRun Run = Fly(Selection.ChooseView(Chosen, UghShapes::ToWorld(0, 182, 0).Z), Shift, false, Isles,
			FVector::ZeroVector, After);
		const FString Name = FString::Printf(TEXT("the selection to level %d"), Chosen + 1);
		TestTrue(Name + TEXT(": starts at the choosing camera"), Run.bStartsThere);
		TestTrue(Name + TEXT(": ends exactly in the game's camera"), Run.bEndsThere && Run.MistAtEnd <= 0);
		TestTrue(Name + TEXT(": the world switches in a thick mist"), Run.bMistAtSwitch);
		TestTrue(FString::Printf(TEXT("%s: above the sea (%.0f)"), *Name, Run.Lowest), Run.Lowest >= AboveSea);
		TestTrue(FString::Printf(TEXT("%s: clear of the stones (%.0f)"), *Name, Run.Nearest), Run.Nearest >= Clear);
		TestTrue(FString::Printf(TEXT("%s: no jolt (%.0f units/s2 a frame at %.2f s)"), *Name, Run.Jolt, Run.JoltAt), Run.Jolt <= MaxJolt);
		TestTrue(FString::Printf(TEXT("%s: turns smoothly (%.2f degrees a frame at %.2f s)"), *Name, Run.Turn, Run.TurnAt), Run.Turn <= MaxTurn);
	}
	// a key before the mist: into it at once, then the rest in HurrySeconds from SkipTo before the end; after it the
	// rest in HurrySeconds
	{
		const FVector Before = Stone - Isles[3].Foot, After = Stone - Isles[4].Foot;
		const FRun Early = Fly(Game, FVector(Before - After).GetSafeNormal2D() * (Before - After).Size2D(), true, Isles,
			Before, After, 0.5);
		TestTrue(FString::Printf(TEXT("hurried early: over soon (%.2f s)"), Early.Took), Early.Took <=
			0.5 + FUghVoyage::MistHurried + SwitchFrames * Frame + FUghVoyage::HurrySeconds + 0.1);
		TestTrue(TEXT("hurried early: ends in the game's camera"), Early.bEndsThere && Early.bMistAtSwitch);
		const FRun Late = Fly(Game, FVector(Before - After).GetSafeNormal2D() * (Before - After).Size2D(), true, Isles,
			Before, After, 6);
		TestTrue(FString::Printf(TEXT("hurried late: over soon (%.2f s)"), Late.Took),
			Late.Took <= 6 + FUghVoyage::HurrySeconds + 0.05);
		TestTrue(TEXT("hurried late: ends in the game's camera"), Late.bEndsThere);
	}
	return true;
}

#endif
