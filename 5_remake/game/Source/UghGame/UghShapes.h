// Grey boxes on the plane of the play.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

class AActor;
class UInstancedStaticMeshComponent;

/**
 * Where the screen of the original lies in the world, and the boxes drawn on it. The plane of the play is X (right)
 * and Z (up); the screen's 320 x 192 px are 3200 x 1920 units, its top left corner at Z = 1920. A depth is
 * negative towards the camera, which looks along -Y (world Y = -depth), so X is to its right.
 */
namespace UghShapes
{
	/** World units per pixel of the original's screen. */
	constexpr double UnitsPerPixel = 10.0;

	/** The values of ugh_logic.h as numbers (its enums must not mix with floating point). */
	constexpr int32 ScreenWidth = UGH_LOGIC_SCREEN_WIDTH, ScreenHeight = UGH_LOGIC_SCREEN_HEIGHT;
	constexpr int32 Subpixels = UGH_LOGIC_SUBPIXELS, FadeShown = UGH_LOGIC_FADE_SHOWN;
	constexpr int32 CopterBodyLeft = UGH_LOGIC_COPTER_BODY_LEFT, CopterBodyRight = UGH_LOGIC_COPTER_BODY_RIGHT,
		CopterBodyHeight = UGH_LOGIC_COPTER_BODY_HEIGHT;

	/** The point of the world at pixel x, y of the screen (fractions allowed) and `Depth` units. */
	FVector ToWorld(double X, double Y, double Depth);

	/** A box from pixel `Left`, `Top` of the screen, `Width` x `Height` px, centred at `Depth`, `Thickness` units deep. */
	FTransform Box(double Left, double Top, double Width, double Height, double Depth, double Thickness);

	/** New boxes of one colour (engine cube, the basic shape material) on `Owner`, no collision. */
	UInstancedStaticMeshComponent* AddBoxes(AActor* Owner, const FLinearColor& Color);

	/** Shows exactly `Boxes` (moves the instances there, adds or removes when the count differs). */
	void SetBoxes(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Boxes);
}
