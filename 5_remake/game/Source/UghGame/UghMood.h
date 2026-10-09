// The time of day and the weather of a level.
#pragma once

#include "CoreMinimal.h"

/**
 * The light and the air of a level (AUghStage): a day, a golden evening, a dusk, a night lit by the moon and the
 * campfires, or a storm (a level with wind, ugh_logic_view.wind). The calm levels go through the day in the order of
 * the mode (ugh_logic_view.level): the same level, the same mood.
 */
struct FUghMood
{
	enum class EKind : uint8 { Day, Evening, Dusk, Night, Storm };

	EKind Kind;
	const TCHAR* Name;
	/** The sun (or the moon): how high (degrees above the horizon), from where (yaw), its colour and lux. */
	float SunPitch, SunYaw;
	FLinearColor SunColor;
	float SunLux;
	/** The sky: its picture (an id of UghAssets), tinted, as bright; the sky light that captures it. */
	const TCHAR* Sky;
	FLinearColor SkyTint;
	float SkyLight;
	/** The fog: its density and colour; how much the sunlight lights it up (shafts of light in the volumetric fog). */
	float FogDensity;
	FLinearColor FogColor;
	float Shafts;
	/** A mist over the water (density, 0 none). */
	float Mist;
	/** The fixed exposure, EV100: lower for a darker mood, so that it stays readable. */
	float Exposure;
	/** How bright the caustics under the water are. */
	float Caustics;
	/**
	 * How bright the sky looks where it is seen (flying to the stone, FUghIntro): as bright as the light it gives (the
	 * sky light), but at night (the night's picture is as bright as a day's); the sky light captures it as it is.
	 */
	float SkySeen;
	/** How bright the campfires' and torches' lights are, times their brightness by day: the main light in the dark. */
	float FireLight;
	/**
	 * The figures stand out (UghFigureLook): a soft light on them alone from the front right (and a rim from behind them,
	 * AUghStage), this times the light the exposure makes mid grey (2^Exposure lux); how much the background around them
	 * darkens unless it is dark already (their halo, 0 .. 1).
	 */
	float FigureFill;
	float Halo;
	/**
	 * The light in the shade of the cave as the exposure shows it (linear, what a white surface there looks like): the
	 * puffs of the bursts take it (UghMaterials::Burst) besides the sun, the fires and the flashes reaching them - the
	 * engine's indirect light of translucency does not reach the play, 113 m from its camera.
	 */
	FLinearColor Shade;
};

namespace UghMood
{
	/** How many calm levels one day takes (UghMood::Of). */
	constexpr int32 LevelsADay = 6;

	/** The mood of level `Level` (from 0 in the order of the mode) with `Wind` (-1, 1 a storm, 0 calm). */
	const FUghMood& Of(int32 Level, int32 Wind);
}
