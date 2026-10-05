// The screen as automation tests of the editor: the pictures carved in stone (Ugh.Ui.Pictures; a look at them in
// Saved/Ui) and the camera behind the menu (Ugh.Ui.MenuView).
#if WITH_DEV_AUTOMATION_TESTS

#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UghMenuView.h"
#include "UghShapes.h"
#include "UghStage.h"
#include "UghStoneArt.h"

namespace
{
	struct FPicture
	{
		const TCHAR* Name;
		int32 Width, Height;
		TArray<FColor> (*Make)(int32, int32);
		/** How much of it the shape covers at least and at most. */
		double Least, Most;
	};

	/** Units: the camera behind the menu is this far from the stone's middle at least, this high above the sea. */
	constexpr double MenuFar = 20000, MenuAbove = 1000;
	/** Degrees: it looks at most this far beside the stone's middle; it sees the face from at most this far aside. */
	constexpr double MenuAside = 15, MenuAround = 50;
	/** A frame, seconds; the camera moves less than this a frame (units). */
	constexpr double Frame = 1.0 / 60, MenuStep = 100;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghUiPicturesTest, "Ugh.Ui.Pictures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghUiPicturesTest::RunTest(const FString& Parameters)
{
	const FPicture Pictures[] = {
		{ TEXT("logo"), 1100, 420, &UghStoneArt::Logo, 0.15, 0.5 },
		{ TEXT("tablet"), 1000, 330, &UghStoneArt::Tablet, 0.5, 0.85 },
		{ TEXT("copter"), 96, 72, &UghStoneArt::Copter, 0.15, 0.6 },
		{ TEXT("bone"), 640, 80, &UghStoneArt::Bone, 0.35, 0.8 },
	};
	for (const FPicture& Picture : Pictures)
	{
		const TArray<FColor> Pixels = Picture.Make(Picture.Width, Picture.Height);
		if (!TestEqual(FString::Printf(TEXT("%s: its size"), Picture.Name), Pixels.Num(), Picture.Width * Picture.Height))
		{
			continue;
		}
		const int32 Covered = Pixels.FilterByPredicate([](const FColor& Pixel) { return Pixel.A > 200; }).Num();
		const double Cover = double(Covered) / Pixels.Num();
		TestTrue(FString::Printf(TEXT("%s: the shape covers %.2f"), Picture.Name, Cover),
			Cover >= Picture.Least && Cover <= Picture.Most);
		TestTrue(FString::Printf(TEXT("%s: clear in its corner"), Picture.Name), Pixels[0].A < 10);
		// lit and shaded: not one flat colour
		int32 Darkest = 255, Lightest = 0;
		for (const FColor& Pixel : Pixels)
		{
			if (Pixel.A > 250)
			{
				Darkest = FMath::Min<int32>(Darkest, Pixel.G);
				Lightest = FMath::Max<int32>(Lightest, Pixel.G);
			}
		}
		TestTrue(FString::Printf(TEXT("%s: shaded"), Picture.Name), Lightest - Darkest > 60);
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Picture.Width, Picture.Height, Pixels, Png);
		FFileHelper::SaveArrayToFile(Png, *(FPaths::ProjectSavedDir() / TEXT("Ui") / Picture.Name + TEXT(".png")));
	}
	TestEqual(TEXT("the fade: dark on its left, clear on its right"), UghStoneArt::Fade(64, 0.8f)[0].A, uint8(204));
	TestEqual(TEXT("the fade: clear on its right"), UghStoneArt::Fade(64, 0.8f)[63].A, uint8(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUghUiMenuViewTest, "Ugh.Ui.MenuView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUghUiMenuViewTest::RunTest(const FString& Parameters)
{
	const FUghCameraPose Game = AUghStage::Fit(UghShapes::Screen(), 16.0 / 9);
	const double SeaZ = UghShapes::ToWorld(0, 182, 0).Z;
	const FVector Stone = UghMenuView::StoneMiddle();
	const FVector Face = -Game.Rotation.Vector().GetSafeNormal2D();   // the way the stone's face looks
	FVector Last = UghMenuView::At(Game, SeaZ, 0).Location;
	bool bFar = true, bAbove = true, bLooks = true, bFront = true, bSmooth = true;
	for (double Time = 0; Time < UghMenuView::Period; Time += Frame)
	{
		const FUghCameraPose Pose = UghMenuView::At(Game, SeaZ, Time);
		const FVector To = Stone - Pose.Location;
		bFar &= To.Size() > MenuFar;
		bAbove &= Pose.Location.Z > SeaZ + MenuAbove;
		bLooks &= FMath::RadiansToDegrees(FMath::Acos(To.GetSafeNormal() | Pose.Rotation.Vector())) < MenuAside;
		bFront &= FMath::RadiansToDegrees(FMath::Acos(-To.GetSafeNormal2D() | Face)) < MenuAround;
		bSmooth &= FVector::Distance(Pose.Location, Last) < MenuStep;
		Last = Pose.Location;
	}
	TestTrue(TEXT("the menu's camera far from the stone"), bFar);
	TestTrue(TEXT("above the sea"), bAbove);
	TestTrue(TEXT("looking at the stone (a little beside it)"), bLooks);
	TestTrue(TEXT("in front of its carved face"), bFront);
	TestTrue(TEXT("moving smoothly"), bSmooth);
	return true;
}

#endif
