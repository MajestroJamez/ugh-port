// The start of a level as automation tests of the editor: the flight over the sea ends in the game's camera, low over
// the water and never into the stone (Ugh.Intro); the stone holds the level's rock and stays out of the game's view
// (Ugh.Stack).
#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Count.h"
#include "Misc/AutomationTest.h"
#include "UghBackground.h"
#include "UghIntro.h"
#include "UghRockMesh.h"
#include "UghStackField.h"
#include "UghStage.h"

namespace
{
	/** The aspects of a window the game is played in (the game's camera fits the screen to them). */
	constexpr double Aspects[] = { 16.0 / 9, 16.0 / 10 };
	/** The water at a level's start: low, and high. */
	constexpr double Surfaces[] = { 182, 120 };
	constexpr double Frame = 1.0 / 120;
	/**
	 * Units: the flight starts at least this far from the end (and this high above the sea at most), stays this high
	 * above the sea and this far from the stone (pixels), its speed below this, its turn below this (degrees a frame);
	 * it stops softly.
	 */
	constexpr double FarAway = 15000, StartHigh = 600, AboveSea = 100, FromStone = 20, MaxSpeed = 12000, MaxTurn = 2,
		StopSpeed = 200;
	/**
	 * Pixels: the way to a point of the stone is looked along in these steps up to this near it; a plant's foot is
	 * at most this far from its surface.
	 */
	constexpr double RayStep = 2, RayEnd = 4, OnStone = 4;
	/** The caption's fade-out and the black before the play: 65 + 8 frames of the logic (its phases). */
	constexpr double CaptionToPlay = (65 + 8) / 70.086;

	/** A point of the world in pixels of the screen and depth pixels (FUghStackField's). */
	FVector Pixels(const FVector& World)
	{
		return FVector(World.X / UghShapes::UnitsPerPixel, UghShapes::ScreenHeight - World.Z / UghShapes::UnitsPerPixel,
			-World.Y / UghShapes::UnitsPerPixel);
	}

	/** Whether `At` (pixels) is in the stone's hollow behind the level's rock's face (in the rock or behind it). */
	bool InHollow(const FVector& At)
	{
		return At.Z >= FUghRockField::FrontDepth &&
			At.X > FUghStackField::HoleLeft - 1 && At.X < FUghStackField::HoleRight + 1 &&
			At.Y > FUghStackField::HoleTop - 1 && At.Y < FUghStackField::HoleBottom + 1;
	}

	/** Whether the camera at `Pose` sees `Point` (the world) in a view of `Aspect`. */
	bool Sees(const FUghCameraPose& Pose, double Aspect, const FVector& Point)
	{
		const FRotationMatrix Axes(Pose.Rotation);
		const FVector To = Point - Pose.Location;
		const double Ahead = To | Axes.GetUnitAxis(EAxis::X);
		const double HalfTan = FMath::Tan(FMath::DegreesToRadians(Pose.FieldOfView / 2));
		return Ahead > 0 && FMath::Abs(To | Axes.GetUnitAxis(EAxis::Y)) <= Ahead * HalfTan &&
			FMath::Abs(To | Axes.GetUnitAxis(EAxis::Z)) <= Ahead * HalfTan / Aspect;
	}

	/**
	 * Whether the game's camera at `Pose` sees `Point` of the stone (the world) in a view of `Aspect`: in its view,
	 * neither in the hollow behind the level's rock's face nor behind the stone itself (its way there through the
	 * stone's field).
	 */
	bool SeesOfStone(const FUghStackField& Field, const FUghCameraPose& Pose, double Aspect, const FVector& Point)
	{
		const FVector At = Pixels(Point);
		if (!Sees(Pose, Aspect, Point) || InHollow(At))
		{
			return false;
		}
		const FVector From = Pixels(Pose.Location);
		const double Length = FVector::Dist(From, At);
		for (double Along = 0; Along < Length - RayEnd; Along += RayStep)
		{
			if (Field.Sample(FMath::Lerp(From, At, Along / Length)) > 0)
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghIntroTest, "Ugh.Intro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghIntroTest::RunTest(const FString& Parameters)
{
	for (const double Aspect : Aspects)
	{
		for (const double Surface : Surfaces)
		{
			const FUghCameraPose End = AUghStage::Fit(UghShapes::Screen(), Aspect);
			const double SeaZ = UghShapes::ToWorld(0, Surface, 0).Z;
			const FString Case = FString::Printf(TEXT("aspect %.2f, water at %.0f px: "), Aspect, Surface);
			const FUghCameraPose Last = FUghIntro::At(End, SeaZ, FUghIntro::Duration);
			TestTrue(Case + TEXT("it ends in the game's camera"), Last.Location == End.Location &&
				Last.Rotation == End.Rotation && Last.FieldOfView == End.FieldOfView);
			const FVector Forward = FRotator(0, End.Rotation.Yaw, 0).Vector();
			const FUghCameraPose First = FUghIntro::At(End, SeaZ, 0);
			TestTrue(Case + TEXT("it starts far away"), ((End.Location - First.Location) | Forward) >= FarAway);
			TestTrue(Case + TEXT("low over the sea"), First.Location.Z - SeaZ <= StartHigh);
			TestEqual(Case + TEXT("with a wide lens"), First.FieldOfView, FUghIntro::StartFieldOfView);

			double Lowest = UE_BIG_NUMBER, Nearest = UE_BIG_NUMBER, Past = -UE_BIG_NUMBER, Fastest = 0, Turn = 0, Roll = 0,
				Stop = 0;
			bool bLens = true;
			FUghCameraPose Previous = First;
			for (double Time = Frame; Time <= FUghIntro::Duration + Frame / 2; Time += Frame)
			{
				const FUghCameraPose Pose = FUghIntro::At(End, SeaZ, Time);
				Lowest = FMath::Min(Lowest, Pose.Location.Z - SeaZ);
				Nearest = FMath::Min(Nearest, -FUghStackField::Value(Pixels(Pose.Location)));
				Past = FMath::Max(Past, (Pose.Location - End.Location) | Forward);
				const double Speed = FVector::Dist(Pose.Location, Previous.Location) / Frame;
				Fastest = FMath::Max(Fastest, Speed);
				Stop = Time > FUghIntro::Duration - 0.25 ? FMath::Max(Stop, Speed) : Stop;
				Turn = FMath::Max(Turn, FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
					Pose.Rotation.Vector() | Previous.Rotation.Vector(), -1.0, 1.0))));
				Roll = FMath::Max(Roll, FMath::Abs(Pose.Rotation.Roll));
				bLens &= Pose.FieldOfView <= Previous.FieldOfView + 1e-4f && Pose.FieldOfView >= End.FieldOfView;
				Previous = Pose;
			}
			TestTrue(Case + FString::Printf(TEXT("above the sea (%.0f units at the lowest)"), Lowest), Lowest >= AboveSea);
			TestTrue(Case + FString::Printf(TEXT("away from the stone (%.0f px)"), Nearest), Nearest >= FromStone);
			TestTrue(Case + TEXT("never past the game's camera"), Past <= 1);
			TestTrue(Case + FString::Printf(TEXT("smooth: %.0f units/s at most"), Fastest), Fastest < MaxSpeed);
			TestTrue(Case + FString::Printf(TEXT("smooth: %.2f degrees a frame at most"), Turn), Turn < MaxTurn);
			TestTrue(Case + FString::Printf(TEXT("banking %.1f degrees at most"), Roll), Roll <= FUghIntro::MaxRoll + 1e-3);
			TestTrue(Case + FString::Printf(TEXT("a soft stop (%.0f units/s)"), Stop), Stop < StopSpeed);
			TestTrue(Case + TEXT("the lens narrows to the game's"), bLens);
		}
	}

	// the clock: a long frame (the level being built) moves it a little, a key hurries it before the play begins
	FUghIntro Intro;
	TestFalse(TEXT("no flight before it starts"), Intro.IsFlying());
	Intro.Start();
	for (int32 Settle = 0; Settle < FUghIntro::SettleFrames; ++Settle)
	{
		Intro.Advance(1.0 / 60);
	}
	TestTrue(TEXT("black and still while the renderer settles"), Intro.Shown() == 0 && Intro.GetTime() == 0);
	Intro.Advance(2);
	TestEqual(TEXT("a long frame"), Intro.GetTime(), FUghIntro::MaxStep);
	while (Intro.GetTime() < 1)
	{
		Intro.Advance(1.0 / 60);
	}
	TestEqual(TEXT("shown"), Intro.Shown(), 1.0);
	Intro.Hurry();
	double Hurried = 0;
	while (Intro.IsFlying() && Hurried < 10)
	{
		Intro.Advance(1.0 / 60);
		Hurried += 1.0 / 60;
	}
	TestTrue(FString::Printf(TEXT("hurried to its end in %.2f s"), Hurried), Hurried <= FUghIntro::HurrySeconds + 0.02);
	TestEqual(TEXT("at its end"), Intro.GetTime(), FUghIntro::Duration);
	TestTrue(TEXT("before the play can begin"), FUghIntro::HurrySeconds < CaptionToPlay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghStackTest, "Ugh.Stack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghStackTest::RunTest(const FString& Parameters)
{
	FUghStackField Field;
	Field.Build();
	TArray<FVector> Points;
	TArray<FUghNetQuad> Quads;
	UghSurfaceNets::Build(Field, Points, Quads);
	FUghRockMesh Mesh;
	Mesh.Build(Points, Quads, [](const FVector& Point) { return -FUghStackField::Gradient(Point).GetSafeNormal(); },
		&FUghStackField::Shade);
	TestTrue(TEXT("a stone"), Mesh.Triangles.Num() > 10000);

	// the level's rock sits in its hollow: the rock's open border inside the stone, from the rock's face back
	int32 Outside = 0;
	const TConstArrayView<double> Depths = FUghRockField::Depths();
	for (int32 K = 0; K < Depths.Num(); ++K)
	{
		if (Depths[K] < FUghRockField::FrontDepth)
		{
			continue;
		}
		for (int32 J = 0; J < FUghRockField::Rows; ++J)
		{
			for (int32 I = 0; I < FUghRockField::Columns; ++I)
			{
				const bool bBorder = I == 0 || J == 0 || I == FUghRockField::Columns - 1 || J == FUghRockField::Rows - 1;
				Outside += bBorder && Field.Sample(FUghRockField::Node(I, J, K)) <= 0;
			}
		}
	}
	TestEqual(TEXT("the rock's border in the stone"), Outside, 0);
	// and the shroud that shades the cave
	Outside = 0;
	for (const FTransform& Box : AUghBackground::ShroudBoxes())
	{
		for (int32 Corner = 0; Corner < 125; ++Corner)
		{
			const FVector Unit(Corner % 5, Corner / 5 % 5, Corner / 25);
			const FVector At = Pixels(Box.TransformPosition((Unit / 4 - 0.5) * UghShapes::ShapeSize));
			if (!InHollow(At) && Field.Sample(At) <= 0 && Outside++ < 5)
			{
				AddInfo(FString::Printf(TEXT("the shroud out of the stone at %s"), *At.ToString()));
			}
		}
	}
	TestEqual(TEXT("the shroud in the stone"), Outside, 0);

	// out of the game's view, its jungle too
	const TArray<FUghDecoration> Plants = Field.Plants(Points);
	for (const double Aspect : Aspects)
	{
		const FUghCameraPose Game = AUghStage::Fit(UghShapes::Screen(), Aspect);
		int32 Seen = 0;
		for (const FVector& Vertex : Mesh.Vertices)
		{
			if (SeesOfStone(Field, Game, Aspect, Vertex) && Seen++ < 5)
			{
				AddInfo(FString::Printf(TEXT("seen at %s"), *Pixels(Vertex).ToString()));
			}
		}
		TestEqual(FString::Printf(TEXT("aspect %.2f: none of the stone seen"), Aspect), Seen, 0);
		const int32 PlantsSeen = Algo::CountIf(Plants, [&](const FUghDecoration& Plant)
		{
			return SeesOfStone(Field, Game, Aspect, UghShapes::ToWorld(Plant.X, Plant.Top(), Plant.Depth));
		});
		TestEqual(FString::Printf(TEXT("aspect %.2f: none of its plants seen"), Aspect), PlantsSeen, 0);
	}

	// a jungle on it, the same every time
	const int32 Palms = Algo::CountIf(Plants, [](const FUghDecoration& Plant)
	{
		return Plant.Kind == FUghDecoration::EKind::Palm;
	});
	TestTrue(FString::Printf(TEXT("%d plants, %d palms"), Plants.Num(), Palms), Plants.Num() >= 300 && Palms >= 40);
	int32 Floating = 0;
	for (const FUghDecoration& Plant : Plants)
	{
		const FVector Foot(Plant.X, Plant.Y, Plant.Depth / UghShapes::UnitsPerPixel);
		if (FMath::Abs(Field.Sample(Foot)) > OnStone && Floating++ < 5)
		{
			AddInfo(FString::Printf(TEXT("a plant off the stone at %s (%.1f)"), *Foot.ToString(), Field.Sample(Foot)));
		}
	}
	TestEqual(TEXT("every plant on the stone"), Floating, 0);
	TestTrue(TEXT("the same plants every time"), Field.Plants(Points) == Plants);
	return true;
}

#endif
