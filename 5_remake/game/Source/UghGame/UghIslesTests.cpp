// The level selection over the archipelago as automation tests of the editor (Ugh.Isles.*): the progress kept in the
// profile, the stones' places, the cursor, which stones can be chosen, the passwords, the flights and their keys, the
// level the chosen stone starts.
#if WITH_DEV_AUTOMATION_TESTS

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UghControls.h"
#include "UghIsles.h"
#include "UghJson.h"
#include "UghMenu.h"
#include "UghMenuView.h"
#include "UghPasswords.h"
#include "UghProfile.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghStage.h"
#include "UghWater.h"

namespace
{
	const FString Assets() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../../assets")); }

	FString ProfileFile(const TCHAR* Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / Name);
	}

	bool LoadPasswords(FUghPasswords& Passwords, FString& Error)
	{
		const FString Path = Assets() / UghJson::LevelsFile;
		TSharedPtr<FJsonObject> Levels;
		return UghJson::ReadObject(Path, Levels, Error) && Passwords.Load(*Levels, Path, Error);
	}

	/** A frame of 60 a second. */
	constexpr double Frame = 1.0 / 60;
	/** The sea of the tests (the world's z) and the game's camera over it. */
	constexpr double SeaZ = 150;
	FUghCameraPose Game() { return AUghStage::Play(16.0 / 9); }

	/**
	 * Runs the level selection of `Menu` until it is in `Stage`, it starts a game (into `Chosen`) or `Seconds` pass;
	 * the seconds it took.
	 */
	double RunUntil(FUghMenu& Menu, FUghIsles::EStage Stage, double Seconds, TOptional<FUghGameChoice>* Chosen = nullptr)
	{
		double Took = 0;
		while (Menu.GetIsles().GetStage() != Stage && Took < Seconds)
		{
			const TOptional<FUghGameChoice> Choice = Menu.AdvanceIsles(Frame, Game(), SeaZ);
			Took += Frame;
			if (Choice)
			{
				if (Chosen)
				{
					*Chosen = Choice;
				}
				break;
			}
		}
		return Took;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghIslesProgressTest, "Ugh.Isles.Progress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The levels done of each mode are kept in the profile next to the level its last game got to, saved and read back the
 * same; a profile from before the level selection (no levels done) has the levels before its last one done; the
 * stones' colours follow.
 */
bool FUghIslesProgressTest::RunTest(const FString& Parameters)
{
	FUghProfile Saved;
	for (const int32 Level : { 4, 0, 2, 1, 3, 2 })
	{
		Saved.Scores.SetDone(1, Level);
	}
	TestTrue(TEXT("done once each, sorted"), Saved.Scores.Done(1) == TArray<int32>({ 0, 1, 2, 3, 4 }));
	TestFalse(TEXT("again: no change"), Saved.Scores.SetDone(1, 3));
	TestFalse(TEXT("no level out of range"), Saved.Scores.SetDone(1, -1) || Saved.Scores.SetDone(1, 5000));
	Saved.Scores.SetDone(2, 40);
	Saved.Scores.SetLastLevel(1, 5);
	Saved.Scores.SetLastLevel(2, 41);
	const FString Path = ProfileFile(TEXT("UghIsles.json"));
	FString Error;
	FUghProfile Loaded;
	TestTrue(TEXT("saved and read: ") + Error, Saved.Save(Path, Error) && Loaded.Load(Path, Error));
	TestTrue(TEXT("the same progress read back"), Loaded == Saved && Loaded.Scores.Done(2) == TArray<int32>({ 40 }));

	// a profile of step 22: the levels before the last ones done, nothing in a mode never played
	const FString Old = ProfileFile(TEXT("UghOld.json"));
	FFileHelper::SaveStringToFile(TEXT("{ \"version\": 1, \"settings\": { \"volume\": 70 }, \"highScores\": {")
		TEXT(" \"onePlayer\": { \"lastLevel\": 12, \"best\": [ { \"name\": \"GROG\", \"score\": 900, \"level\": 12 } ] },")
		TEXT(" \"team\": { \"lastLevel\": -1, \"best\": [] }, \"lastName\": \"GROG\" } }"), *Old);
	TestTrue(TEXT("an old profile read: ") + Error, Loaded.Load(Old, Error));
	TestTrue(TEXT("its settings and high scores as they were"), Loaded.Settings.Volume == 70 &&
		Loaded.Scores.Table(1).Num() == 1 && Loaded.Scores.LastLevel(1) == 12);
	TArray<int32> Before;
	for (int32 Level = 0; Level < 12; ++Level)
	{
		Before.Add(Level);
	}
	TestTrue(TEXT("the levels before its last one done"), Loaded.Scores.Done(1) == Before);
	TestTrue(TEXT("none of a mode never played"), Loaded.Scores.Done(2).IsEmpty());
	TestEqual(TEXT("its stone got to: the last level"), FUghIsles::Current(Loaded.Scores, 1, 69), 12);
	TestTrue(TEXT("green, yellow, red"), FUghIsles::StateOf(Loaded.Scores, 1, 11, 69) == EUghIsle::Done &&
		FUghIsles::StateOf(Loaded.Scores, 1, 12, 69) == EUghIsle::Open &&
		FUghIsles::StateOf(Loaded.Scores, 1, 13, 69) == EUghIsle::Locked);
	TestTrue(TEXT("a fresh profile: only the first level open"), FUghIsles::StateOf(FUghHighScores(), 1, 0, 69) ==
		EUghIsle::Open && FUghIsles::StateOf(FUghHighScores(), 1, 1, 69) == EUghIsle::Locked);
	TestTrue(TEXT("a password opens its level"), FUghIsles::StateOf(FUghHighScores(), 1, 30, 69, 30) == EUghIsle::Open);
	// the furthest done goes on: done up to 20, the last game from 3 got to 5 - level 21 is still open
	FUghHighScores Scores;
	for (int32 Level = 0; Level <= 20; ++Level)
	{
		Scores.SetDone(1, Level);
	}
	Scores.SetLastLevel(1, 5);
	TestEqual(TEXT("got to: after the furthest done"), FUghIsles::Current(Scores, 1, 69), 21);
	// all done: no level is open but done ones
	for (int32 Level = 0; Level < 69; ++Level)
	{
		Scores.SetDone(1, Level);
	}
	TestTrue(TEXT("all done: all green"), FUghIsles::StateOf(Scores, 1, 68, 69) == EUghIsle::Done);
	IFileManager::Get().DeleteDirectory(*FPaths::AutomationTransientDir(), false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghIslesLayoutTest, "Ugh.Isles.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The stones of both modes out at sea in front of the title's stone, in the open sea, apart, in rows from the camera's
 * side - the first level the furthest out, a winding path: each next level beside it or round the row's end above it -,
 * the same every time; the camera choosing and the end of the flight to a stone clear of every stone.
 */
bool FUghIslesLayoutTest::RunTest(const FString& Parameters)
{
	const FVector Home = UghMenuView::StoneMiddle();
	for (const int32 Count : { 69, 81 })
	{
		const FString Mode = FString::Printf(TEXT("%d levels: "), Count);
		const TArray<FUghIslePlace> Places = FUghIsles::Layout(Count, Home);
		TestEqual(Mode + TEXT("a stone a level"), Places.Num(), Count);
		TestTrue(Mode + TEXT("the same every time"), Places[33].Foot == FUghIsles::Layout(Count, Home)[33].Foot);
		const double OpenSea = UghWater::OpenSea - 2 * FUghIsles::StoneRadius;
		double Closest = TNumericLimits<double>::Max();
		bool bPath = true, bOut = true, bInSea = true;
		for (int32 Level = 0; Level < Count; ++Level)
		{
			const FUghIslePlace& Place = Places[Level];
			bInSea &= FMath::Abs(Place.Foot.X - Home.X) < OpenSea && Place.Foot.Y > -OpenSea;
			bOut &= Place.Foot.Y < Home.Y - FUghIsles::Nearest + FUghIsles::RowGap / 2;
			for (int32 Other = Level + 1; Other < Count; ++Other)
			{
				Closest = FMath::Min(Closest, FVector::Dist2D(Place.Foot, Places[Other].Foot));
			}
			if (Level > 0)
			{
				// beside it in the row, or the next row's end on the same side (the path goes round)
				const FUghIslePlace& Last = Places[Level - 1];
				bPath &= (Place.Row == Last.Row && FMath::Abs(Place.Column - Last.Column) == 1) ||
					(Place.Row == Last.Row + 1 && Place.Column == Last.Column);
			}
		}
		TestTrue(Mode + TEXT("in the open sea"), bInSea);
		TestTrue(Mode + TEXT("beyond the title's stone"), bOut);
		TestTrue(Mode + FString::Printf(TEXT("apart (%.0f units)"), Closest), Closest > 4 * FUghIsles::StoneRadius);
		TestTrue(Mode + TEXT("a winding path"), bPath);
		TestTrue(Mode + TEXT("the first level furthest out"), Places[0].Foot.Y > Places.Last().Foot.Y);

		// the camera choosing at any stone and at the end of the flight to it: clear of every stone
		FUghIsles Isles;
		Isles.Open(Count == 81 ? 2 : 1, Count, FUghHighScores(), INDEX_NONE, FUghCameraPose(), Home, SeaZ, false);
		double ClearChoose = TNumericLimits<double>::Max(), ClearEnd = ClearChoose;
		bool bSeen = true, bLow = true, bBeyond = true;
		for (int32 Level = 0; Level < Count; ++Level)
		{
			const FUghCameraPose Choose = Isles.ChooseView(Level, SeaZ), End = Isles.ApproachEnd(Level, Game(), SeaZ);
			for (const FUghIslePlace& Place : Places)
			{
				const double Reach = (FUghIsles::StoneRadius + 1000) * (Place.Scale + 0.15);
				ClearChoose = FMath::Min(ClearChoose, FVector::Dist2D(Choose.Location, Place.Foot) - Reach);
				ClearEnd = FMath::Min(ClearEnd, FVector::Dist2D(End.Location, Place.Foot) - Reach);
			}
			// the stone chosen in the middle of the view
			const FVector Toward = (FUghIsles::Top(Places[Level]) - Choose.Location).GetSafeNormal();
			bSeen &= FMath::RadiansToDegrees(FMath::Acos(Toward | Choose.Rotation.Vector())) < Choose.FieldOfView / 4;
			// low over the waves as the level's flight starts
			// clear of the title's stone behind it
			bBeyond &= End.Location.Y < Home.Y - 10000;
			bLow &= FMath::IsNearlyEqual(End.Location.Z - SeaZ, FUghIntro::At(Game(), SeaZ, 0).Location.Z - SeaZ, 1.0);
		}
		TestTrue(Mode + FString::Printf(TEXT("choosing clear of the stones (%.0f units)"), ClearChoose), ClearChoose > 0);
		TestTrue(Mode + FString::Printf(TEXT("the flight's end clear of the stones (%.0f units)"), ClearEnd), ClearEnd > 0);
		TestTrue(Mode + TEXT("the stone chosen in the middle of the view"), bSeen);
		TestTrue(Mode + TEXT("the flight ends as low as the level's starts"), bLow);
		TestTrue(Mode + TEXT("the flight ends clear of the title's stone"), bBeyond);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghIslesPickTest, "Ugh.Isles.Pick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * The level selection in the menu: PLAY opens it, the cursor on the level got to; arrows (and a gamepad's d-pad) move
 * it from stone to stone; Enter on a red stone does nothing but say why, on a green or yellow one it flies there and
 * starts that level - the same game the level's password starts; a typed password opens its red stone; any key hurries
 * the flights; Esc goes back to the title through black.
 */
bool FUghIslesPickTest::RunTest(const FString& Parameters)
{
	using EStage = FUghIsles::EStage;
	FUghPasswords Passwords;
	FString Error;
	if (!TestTrue(TEXT("passwords: ") + Error, LoadPasswords(Passwords, Error)))
	{
		return false;
	}
	FUghProfile Profile;
	for (int32 Level = 0; Level < 10; ++Level)
	{
		Profile.Scores.SetDone(1, Level);
	}
	Profile.Scores.SetLastLevel(1, 10);
	const FUghDisplayOptions Options;
	FUghMenu Menu(Passwords, Profile, Options);
	TestTrue(TEXT("without it Enter plays"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Play);
	Menu.SetIsles(true);
	TestTrue(TEXT("with it Enter opens it"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Isles);
	const FUghCameraPose Title = UghMenuView::At(Game(), SeaZ, 0);
	Menu.OpenIsles(Title, UghMenuView::StoneMiddle(), SeaZ);
	const FUghIsles& Isles = Menu.GetIsles();
	TestTrue(TEXT("it shows over the archipelago"), Menu.GetScreen() == FUghMenu::EScreen::Isles &&
		Isles.GetStage() == EStage::Arrive && Isles.GetCount() == 69);
	TestEqual(TEXT("the cursor on the level got to"), Isles.GetCursor(), 10);
	TestTrue(TEXT("green, yellow, red"), Isles.GetState(9) == EUghIsle::Done && Isles.GetState(10) == EUghIsle::Open &&
		Isles.GetState(11) == EUghIsle::Locked);

	// the flight over the archipelago: from the title's camera, smooth, to the camera choosing; a key hurries it
	Menu.AdvanceIsles(Frame, Game(), SeaZ);
	TestTrue(TEXT("it starts at the title's camera"), Isles.GetPose().Location.Equals(Title.Location, 50));
	FVector Last = Isles.GetPose().Location;
	double Fastest = 0, Jolt = 0, LastSpeed = 0, LastAccel = 0, Lowest = TNumericLimits<double>::Max();
	for (double Time = 0; Isles.GetStage() == EStage::Arrive && Time < 20; Time += Frame)
	{
		Menu.AdvanceIsles(Frame, Game(), SeaZ);
		const double Speed = FVector::Dist(Last, Isles.GetPose().Location) / Frame;
		const double Accel = (Speed - LastSpeed) / Frame;
		Jolt = Time > 2 * Frame ? FMath::Max(Jolt, FMath::Abs(Accel - LastAccel)) : Jolt;
		Fastest = FMath::Max(Fastest, Speed);
		// over the title's stone high above its jungle
		const FVector At = Isles.GetPose().Location;
		Lowest = FMath::Abs(At.Y - UghMenuView::StoneMiddle().Y) < 7000 ? FMath::Min(Lowest, At.Z) : Lowest;
		Last = Isles.GetPose().Location;
		LastSpeed = Speed;
		LastAccel = Accel;
	}
	TestTrue(TEXT("on to choosing"), Isles.GetStage() == EStage::Choose);
	TestTrue(FString::Printf(TEXT("the flight smooth (its acceleration changing at most %.0f a frame)"), Jolt),
		Jolt < 0.2 * Fastest / Frame);
	TestTrue(TEXT("and stopping"), LastSpeed < 0.02 * Fastest);
	TestTrue(FString::Printf(TEXT("over the title's stone high (%.0f units)"), Lowest), Lowest > 20000);
	Menu.AdvanceIsles(Frame, Game(), SeaZ);
	TestTrue(TEXT("its end the camera choosing"),
		Isles.GetPose().Location.Equals(Isles.ChooseView(10, SeaZ).Location, 1) && Isles.IsSettled());

	// red: no flight, a notice; arrows, a gamepad's d-pad
	// (the second row runs right to left: level 11 left of level 10)
	Menu.HandleKey(EKeys::Left);
	TestEqual(TEXT("Left: the stone left of it, the next level"), Isles.GetCursor(), 11);
	Menu.HandleKey(EKeys::Enter);
	TestTrue(TEXT("Enter on a red stone does nothing"),
		Isles.GetStage() == EStage::Choose && !Isles.GetNotice().IsEmpty());
	Menu.HandleKey(FUghControls::MenuKeyOf(EKeys::Gamepad_DPad_Right));
	Menu.HandleKey(FUghControls::MenuKeyOf(EKeys::Gamepad_DPad_Right));
	TestEqual(TEXT("the d-pad's Right: back along the row"), Isles.GetCursor(), 9);
	Menu.HandleKey(EKeys::Down);
	const FUghIslePlace& Below = Isles.GetPlaces()[Isles.GetCursor()];
	TestTrue(TEXT("Down: the row nearer the camera"), Below.Row == 0 &&
		Isles.GetCursor() == Isles.Neighbour(9, EKeys::Down));
	TestEqual(TEXT("Up: the row beyond"), Isles.GetPlaces()[Isles.Neighbour(Isles.GetCursor(), EKeys::Up)].Row, 1);
	TestEqual(TEXT("round the row's end: the next level"), Isles.Neighbour(8, EKeys::Right), 9);
	TestEqual(TEXT("no stone before the first"), Isles.Neighbour(0, EKeys::Left), 0);
	while (Isles.GetCursor() != 4)
	{
		const bool bRight = Isles.GetPlaces()[Isles.GetCursor()].Column < Isles.GetPlaces()[4].Column;
		Menu.HandleKey(bRight ? EKeys::Right : EKeys::Left);
	}
	// the glide after the cursor: smooth, at rest at the stone
	RunUntil(Menu, EStage::Closed, 4);
	TestTrue(TEXT("the camera glides to the cursor's stone"),
		Isles.IsSettled() && Isles.GetPose().Location.Equals(Isles.ChooseView(4, SeaZ).Location, 50));

	// green: the flight to the stone, then the game of that level - the same as its password's
	TestTrue(TEXT("Enter on a green stone flies"), Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::None &&
		Isles.GetStage() == EStage::Approach);
	const FVector Start = Isles.GetPose().Location;
	TOptional<FUghGameChoice> Chosen;
	Menu.AdvanceIsles(Frame, Game(), SeaZ);
	TestTrue(TEXT("from the camera choosing, from a stop"), FVector::Dist(Start, Isles.GetPose().Location) < 5);
	double Took = RunUntil(Menu, EStage::Arrived, 20, &Chosen);
	Took += RunUntil(Menu, EStage::Closed, 1, &Chosen);
	TestTrue(FString::Printf(TEXT("as long as its way (%.1f s)"), Took), Took >= FUghIsles::ApproachMin - 0.1 &&
		Took <= FUghIsles::ApproachMax + 0.2);
	TestTrue(TEXT("its level starts"), Chosen.IsSet() && Chosen->Players == 1 && Chosen->FirstLevel == 4 &&
		Menu.GetScreen() == FUghMenu::EScreen::Title);
	FUghProfile Plain;
	FUghMenu ByPassword(Passwords, Plain, Options);
	for (const TCHAR Char : Passwords.Get(1, 4))
	{
		ByPassword.HandleKey(FUghMenu::KeyOf(Char));
	}
	TestTrue(TEXT("the game of its password"), Chosen.IsSet() && ByPassword.GetChoice() == *Chosen);
	FUghSimulation Simulation;
	if (TestTrue(TEXT("the logic: ") + Error, Simulation.Load(Assets() / TEXT("logic/ugh-data.ugd"), Error)))
	{
		Simulation.Preview(*Chosen);
		TestTrue(TEXT("the logic plays the chosen level"), Simulation.GetCurrent().level == 4 &&
			Simulation.GetCurrent().level_id >= 0);
	}

	// a typed password opens its red stone; the flight's end where the level's flight starts; a key hurries it
	for (const TCHAR Char : Passwords.Get(1, 30))
	{
		Menu.HandleKey(FUghMenu::KeyOf(Char));
	}
	TestTrue(TEXT("a password of a red level: Enter opens the selection"),
		Menu.HandleKey(EKeys::Enter) == FUghMenu::EAction::Isles);
	Menu.OpenIsles(Title, UghMenuView::StoneMiddle(), SeaZ);
	TestTrue(TEXT("the cursor on its level, open"), Isles.GetCursor() == 30 && Isles.IsSelectable(30) &&
		Isles.GetState(31) == EUghIsle::Locked);
	Menu.AdvanceIsles(Frame, Game(), SeaZ);
	Menu.HandleKey(EKeys::SpaceBar);
	const double Hurried = RunUntil(Menu, EStage::Choose, 10);
	TestTrue(FString::Printf(TEXT("any key hurries the flight over it (%.2f s)"), Hurried),
		Hurried <= FUghIsles::HurrySeconds + 0.05);
	Menu.HandleKey(EKeys::Enter);
	RunUntil(Menu, EStage::Closed, 1.0);
	Menu.HandleKey(EKeys::A);
	Took = RunUntil(Menu, EStage::Arrived, 10, &Chosen);
	TestTrue(FString::Printf(TEXT("any key hurries the flight to the stone (%.2f s)"), Took),
		Took <= FUghIsles::HurrySeconds + 0.05);
	const FUghCameraPose End = Isles.ApproachEnd(30, Game(), SeaZ);
	TestTrue(TEXT("it ends where the level's flight starts, in front of its stone"),
		Isles.GetPose().Location.Equals(End.Location, 1) &&
		Isles.GetPose().Rotation.Equals(FUghIntro::At(Game(), SeaZ, 0).Rotation, 0.5) && Chosen.IsSet() &&
		Chosen->FirstLevel == 30);

	// Esc: back to the title through black
	Menu.HandleKey(EKeys::Enter);
	Menu.OpenIsles(Title, UghMenuView::StoneMiddle(), SeaZ, false);
	Menu.AdvanceIsles(Frame, Game(), SeaZ);
	Menu.HandleKey(FUghControls::BackKey());
	TestTrue(TEXT("B leaves it"), Isles.GetStage() == EStage::Leave);
	RunUntil(Menu, EStage::Closed, 1);
	TestTrue(TEXT("back at the title, black at first"), Menu.GetScreen() == FUghMenu::EScreen::Title &&
		Isles.Shown() < 0.2 && Menu.GetChoice().FirstLevel == 30);
	return true;
}

#endif
