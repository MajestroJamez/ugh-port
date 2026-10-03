// The materials of the diorama: made by the editor's commandlet UghMakeAssets, used by the game.
#pragma once

#include "CoreMinimal.h"

/**
 * The materials the game uses, as asset paths and parameter names: the commandlet UghMakeAssets (module UghEditor)
 * writes them to Content/Generated on every build.ps1, the game makes dynamic instances of them.
 */
namespace UghMaterials
{
	/** Plasticine: the figures. Parameter Color. */
	inline const TCHAR* Clay = TEXT("/Game/Generated/M_UghClay");
	/** The rock and the cave, coloured by the original's drawing of the level (texture parameter Art, mesh UVs). */
	inline const TCHAR* Rock = TEXT("/Game/Generated/M_UghRock");
	/** Translucent water. Parameters Color, Opacity. */
	inline const TCHAR* Water = TEXT("/Game/Generated/M_UghWater");
	/** A glowing flame (unlit, additive). Parameters Color, Intensity. */
	inline const TCHAR* Fire = TEXT("/Game/Generated/M_UghFire");
	/** A sprite of the original on a card (unlit, colour 0 cut out). Texture parameter Art. */
	inline const TCHAR* Sprite = TEXT("/Game/Generated/M_UghSprite");

	inline const TCHAR* ColorParameter = TEXT("Color");
	inline const TCHAR* ArtParameter = TEXT("Art");
	inline const TCHAR* OpacityParameter = TEXT("Opacity");
	inline const TCHAR* IntensityParameter = TEXT("Intensity");
}
