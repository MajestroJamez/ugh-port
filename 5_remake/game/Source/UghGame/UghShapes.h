// Shapes on the plane of the play.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

class AActor;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;

/**
 * Where the screen of the original lies in the world, and the shapes drawn on it. The plane of the play is X (right)
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

	/** The slab of the play (units deep, around depth 0): the figures fill it, the rock is cut at its front. */
	constexpr double PlaneThickness = 40;

	/** The point of the world at pixel x, y of the screen (fractions allowed) and `Depth` units. */
	FVector ToWorld(double X, double Y, double Depth);

	/** A box from pixel `Left`, `Top` of the screen, `Width` x `Height` px, centred at `Depth`, `Thickness` units deep. */
	FTransform Box(double Left, double Top, double Width, double Height, double Depth, double Thickness);

	/** The engine's basic shapes; a Box transform fits any of them into its box. */
	enum class EShape : uint8 { Cube, Cylinder, Sphere, Cone };
	/** They are this many units across. */
	constexpr double ShapeSize = 100;

	/** A dynamic instance of the material at `Path` (UghMaterials); the engine's default one when it is missing. */
	UMaterialInstanceDynamic* Material(UObject* Outer, const TCHAR* Path);
	/** Plasticine of `Color`. */
	UMaterialInstanceDynamic* Clay(UObject* Outer, const FLinearColor& Color);

	/** New shapes of one kind and material on `Owner`, no collision. */
	UInstancedStaticMeshComponent* AddShapes(AActor* Owner, EShape Shape, UMaterialInterface* Material);

	/** Shows exactly `Boxes` (moves the instances there, adds or removes when the count differs). */
	void SetShapes(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Boxes);

	/** A new card on `Owner` that shows a sprite of the original (material Sprite), no collision, no shadow. */
	UStaticMeshComponent* AddCard(AActor* Owner);
	/** Shows `Card` with `Sprite`: a thin box from pixel Left, Top, Width x Height px, at `Depth`. */
	void ShowCard(UStaticMeshComponent* Card, UTexture2D* Sprite, double Left, double Top, double Width, double Height,
		double Depth);
}
