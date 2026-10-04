// The materials of the diorama: made by the editor's commandlet UghMakeAssets, used by the game.
#pragma once

#include "CoreMinimal.h"

/**
 * The materials the game uses, as asset paths and parameter names: the commandlet UghMakeAssets (module UghEditor)
 * writes them to Content/Generated on every build.ps1, the game makes dynamic instances of them.
 */
namespace UghMaterials
{
	inline const TCHAR* ColorParameter = TEXT("Color");
	inline const TCHAR* ArtParameter = TEXT("Art");
	inline const TCHAR* OpacityParameter = TEXT("Opacity");
	inline const TCHAR* IntensityParameter = TEXT("Intensity");
	inline const TCHAR* WindParameter = TEXT("Wind");
	inline const TCHAR* BaseColorParameter = TEXT("BaseColor");
	inline const TCHAR* NormalParameter = TEXT("Normal");
	inline const TCHAR* RoughnessParameter = TEXT("Roughness");
	inline const TCHAR* OcclusionParameter = TEXT("Occlusion");
	inline const TCHAR* HeightParameter = TEXT("Height");
	inline const TCHAR* TilingParameter = TEXT("Tiling");
	inline const TCHAR* SkyParameter = TEXT("Sky");
	inline const TCHAR* SizeParameter = TEXT("Size");
	inline const TCHAR* HeightMaskParameter = TEXT("HeightMask");
	inline const TCHAR* WaterLevelParameter = TEXT("WaterLevel");

	/** Plasticine: the figures. Parameter Color. */
	inline const TCHAR* Clay = TEXT("/Game/Generated/M_UghClay");
	/**
	 * The rock coloured by the original's drawing of the level (texture parameter Art, mesh UVs): the cliff when the
	 * texture sets of Cliff are not imported.
	 */
	inline const TCHAR* Rock = TEXT("/Game/Generated/M_UghRock");
	/**
	 * The cliff (FUghRockMesh): the layers of imported texture sets mapped onto it from three sides (nothing
	 * stretches), blended by the way the surface faces, by its vertex colours (how open, how deep, how near below a top
	 * edge, its large patches) and by the original's drawing (texture parameter Art, mesh UVs, softened), their heights
	 * deciding where one layer gives way to another. Each layer of CliffLayers has the texture parameters <layer><map>
	 * for the maps of CliffMaps, taken from the scanned surfaces of UghElectricDreams::CliffLayers where they were
	 * copied, else from the instance MI_<id> of its texture set (UghAssets::CliffSets); the scalar <layer>Size (metres
	 * one texture covers) and the vector <layer>HeightMask (which channels of its Height map are its relief: red by
	 * default). The scalar WaterLevel is the world's z of the water's surface (the rock is wet at it and under it).
	 * The shader code is Source/UghEditor/Shaders/UghCliff.hlsl.
	 */
	inline const TCHAR* Cliff = TEXT("/Game/Generated/M_UghCliff");
	inline const TCHAR* const CliffLayers[] = { TEXT("Rock"), TEXT("Stone"), TEXT("Grass"), TEXT("Moss"), TEXT("Soil") };
	inline const TCHAR* const CliffMaps[] = { BaseColorParameter, NormalParameter, RoughnessParameter, HeightParameter };
	/** Translucent water. Parameters Color, Opacity. */
	inline const TCHAR* Water = TEXT("/Game/Generated/M_UghWater");
	/**
	 * A campfire's flame on a card (unlit, additive, seen from both sides; card UVs: u across, v down): tongues of fire
	 * licking upwards, sparks above them, each card of instanced ones flickering its own way. Parameters Intensity and
	 * Wind (-1 .. 1, the flame leans that way). The shader code is Source/UghEditor/Shaders/UghFlame.hlsl.
	 */
	inline const TCHAR* Fire = TEXT("/Game/Generated/M_UghFire");
	/** A sprite of the original on a card (unlit, colour 0 cut out). Texture parameter Art. */
	inline const TCHAR* Sprite = TEXT("/Game/Generated/M_UghSprite");
	/**
	 * A PBR surface of an imported texture set (UghAssets: its instance MI_<id>): texture parameters BaseColor, Normal,
	 * Roughness (its green channel), Occlusion (its red channel: a packed AO/roughness/metal map serves both) and
	 * Height (its red channel: the relief, which shifts the others with the view - parallax), the scalar Tiling
	 * (repeats of the textures per UV unit).
	 */
	inline const TCHAR* Pbr = TEXT("/Game/Generated/M_UghPbr");
	/**
	 * The sky around the world: an HDR picture of it (texture parameter Sky, long-lat) seen in every direction, tinted
	 * by Color, times Intensity; unlit, the sky light captures it (it lights the scene and shows in reflections).
	 */
	inline const TCHAR* Sky = TEXT("/Game/Generated/M_UghSky");
}
