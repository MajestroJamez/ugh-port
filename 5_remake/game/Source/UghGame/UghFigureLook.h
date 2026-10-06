// How the figures stand out of the diorama.
#pragma once

#include "CoreMinimal.h"

class USceneComponent;

/**
 * The figures of the play (the people - waiting, walking, swimming, the pilots and the passengers in the copters -,
 * the enemies, the bonus items) stand out of the rock at first glance in every mood and preset: they alone get a soft
 * light from the front right and a rim from behind (AUghStage's figure lights: a lighting channel of their own, no
 * shadows, nothing bounced by Lumen), and they are drawn into the custom depth, from which a post-process
 * (UghMaterials::FigureHalo) darkens the background just around them unless it is dark already.
 */
namespace UghFigureLook
{
	/** The lighting channel only the figures are in (besides the scene's 0). */
	constexpr int32 Channel = 1;

	/**
	 * `Root` and every primitive under it a figure: lit by the figure light too, drawn into the custom depth for its
	 * halo when `bHalo` (not the people in a copter: the copter stands out by itself, a halo would stain it).
	 */
	void Mark(USceneComponent* Root, bool bHalo = true);
}
