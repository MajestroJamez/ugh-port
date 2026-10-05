// Where the model of a figure stands for its sprite.
#pragma once

#include "CoreMinimal.h"
#include "UghFigureActions.h"

/**
 * Where a figure's model stands, which way it looks and how big it is for its sprite, by the origins of the Blender
 * scripts: a person's feet on the bottom of his sprite (in the water his origin at the surface: WaterLine below its
 * top), as tall as a walking passenger's sprite whatever he does; the flyer in the middle of its sprite; the others on
 * its bottom. A figure looking to a side turns a little towards the camera, so that more of it shows.
 */
namespace UghFigurePlace
{
	/**
	 * The people's height standing (cm, the top of the head): walking (a little lower) with their hair they are as tall
	 * as a walking passenger's sprite, 14 px. They are as big walking, waiting, swimming and sitting in a copter
	 * (Blender/copter_layout.py's PERSON_HEIGHT).
	 */
	constexpr double PersonHeight = 145;
	/** The height of the caveman's model of Blender/caveman.py (cm): the game scales him to PersonHeight. */
	constexpr double CavemanHeight = 115;
	/** In the water a caveman's origin (the surface) is this many pixels below the top of his sprite. */
	constexpr double WaterLine = 4;
	/** A passenger coming out of a door comes from this deep behind the slab of the play (units). */
	constexpr double DoorDepth = 60;
	/**
	 * The flyer flies banked, its back this many degrees towards the camera (its wings stretch in depth: a plane seen
	 * edge on), and bigger than its model by FlyerScale (its sprite is as long as its wings are wide).
	 */
	constexpr double FlyerBank = 35, FlyerScale = 1.3;

	/**
	 * The transform of the model of `Action` for its sprite of `Size` px with its top left corner at `At` (pixels),
	 * its action at `Phase` of its loop (coming out of a door, going into one), turned `Spin` radians (a stone tumbling
	 * about its middle in the plane of the play, a bonus item about its upright axis).
	 */
	FTransform Of(const FUghFigureAction& Action, const FVector2D& At, const FIntPoint& Size, double Phase, double Spin);
}
