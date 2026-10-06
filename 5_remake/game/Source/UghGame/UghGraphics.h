// The graphics settings put into the engine.
#pragma once

#include "CoreMinimal.h"
#include "UghSettingsMenu.h"

class UWorld;

/**
 * The graphics of FUghSettings put into the engine. A quality preset (low, medium, high, epic) is the engine's
 * scalability groups at that level (low with the shadows of medium: the diorama lives by the sun's; without Lumen's
 * global illumination) and the heavy features of the diorama at the preset's values (Variables): the volumetric fog
 * (the shafts of sunlight into the cave), the sea's reflections and refraction, Lumen's reflections, the sun's shadow
 * maps, the translucency's lighting volume (rain, flames), the upscaler's resolution, the fires' shadows and their
 * lights licking about, the people's hair (the grooms' level of detail: cards, or helmets). Epic is the diorama as it
 * was made (the values tuned on the Radeon 890M); low is for 60 fps there (README of the game: Performance). The
 * cvars go in as a game setting: above the scalability groups, below the console (a shot's -ExecCmds). The resolution
 * and the window mode go through the engine's game user settings, only once set in the menu.
 */
namespace UghGraphics
{
	/** One of the diorama's console variables at a preset. */
	struct FVariable
	{
		const TCHAR* Name;
		const TCHAR* Values[FUghSettings::QualityLevels];   // low, medium, high, epic
	};
	/** The diorama's variables of the presets. */
	TConstArrayView<FVariable> Variables();

	/** What this computer offers: DLSS and its frame generations, the screen's resolutions; the window now. */
	FUghDisplayOptions Options();
	/** The quality preset for this computer's GPU (FUghSettings::RecommendedQuality), logged. */
	int32 RecommendedQuality();
	/** The quality preset (0 .. 3): the scalability groups, the variables, the grooms in `World`. */
	void ApplyQuality(int32 Quality, UWorld* World);
	/** The level of detail of the people's grooms at the quality applied last (FUghMetaHuman). */
	int32 GroomLOD();
	/** The resolution and the window mode, when set (else the engine's as it started). */
	void ApplyDisplay(const FUghSettings& Settings);
}
