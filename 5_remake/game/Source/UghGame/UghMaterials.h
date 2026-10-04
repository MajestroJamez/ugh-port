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
	/**
	 * A PBR surface of an imported texture set (UghAssets: its instance MI_<id>): texture parameters BaseColor, Normal,
	 * Roughness (its green channel) and Occlusion (its red channel: a packed AO/roughness/metal map serves both), the
	 * scalar Tiling (repeats of the textures per UV unit).
	 */
	inline const TCHAR* Pbr = TEXT("/Game/Generated/M_UghPbr");

	inline const TCHAR* ColorParameter = TEXT("Color");
	inline const TCHAR* ArtParameter = TEXT("Art");
	inline const TCHAR* OpacityParameter = TEXT("Opacity");
	inline const TCHAR* IntensityParameter = TEXT("Intensity");
	inline const TCHAR* BaseColorParameter = TEXT("BaseColor");
	inline const TCHAR* NormalParameter = TEXT("Normal");
	inline const TCHAR* RoughnessParameter = TEXT("Roughness");
	inline const TCHAR* OcclusionParameter = TEXT("Occlusion");
	inline const TCHAR* TilingParameter = TEXT("Tiling");
}
