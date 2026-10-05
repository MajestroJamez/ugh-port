// The camera of the title screen.
#pragma once

#include "CoreMinimal.h"
#include "UghIntro.h"

/**
 * The camera behind the menu: far out over the sea it swings slowly to and fro around the stone the level is carved
 * into (AUghSeaStack), its carved face always towards it, the stone right of the middle (the menu is on the left),
 * the exposure of the daylight outside as in the flight (FUghIntro).
 */
namespace UghMenuView
{
	/** Seconds of one swing there and back. */
	constexpr double Period = 70;

	/** The middle of the stone, the world. */
	FVector StoneMiddle();
	/** The camera `Time` seconds into the menu; `Game` is the game's camera, `SeaZ` the sea's surface (the world). */
	FUghCameraPose At(const FUghCameraPose& Game, double SeaZ, double Time);
}
