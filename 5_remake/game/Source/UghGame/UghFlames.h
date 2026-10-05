// The flames of the campfires and torches: simulated offline and baked into flipbooks.
#pragma once

#include "CoreMinimal.h"

/**
 * The flipbooks of the flames (the commandlet UghMakeFlames, run by build.ps1): a fire simulated as a gas (FUghFireSim:
 * fuel burning into heat that rises, swirls and cools) at a fixed step, its light laid out as Columns x Rows frames of
 * CellWidth x CellHeight pixels (the first frame top left, row by row; the foot of the flame at the bottom of each),
 * looping. The material UghMaterials::Flame plays them on cards.
 */
namespace UghFlames
{
	constexpr int32 Columns = 8, Rows = 8, Frames = Columns * Rows, CellWidth = 128, CellHeight = 256;
	/** The frames a second the simulation takes (and the flipbook plays). */
	constexpr double FramesPerSecond = 30;
	/** The last Blend frames of the simulation fade into its first ones: the flipbook loops without a jump. */
	constexpr int32 Blend = 16;

	/** The flipbooks (textures made by UghMakeFlames): the campfire's broad flame, the torch's small one. */
	inline const TCHAR* Campfire = TEXT("/Game/Generated/T_UghFlameCampfire");
	inline const TCHAR* Torch = TEXT("/Game/Generated/T_UghFlameTorch");
}
