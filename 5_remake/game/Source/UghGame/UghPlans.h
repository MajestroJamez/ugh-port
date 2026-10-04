// The parts of UghDecorations::Plan, each adding a kind of decorations to a level.
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

class FUghPlacer;

/** Each adds its decorations where FUghPlacer lets them stand, in the order of Plan (the bigger first). */
namespace UghPlans
{
	/** Campfires (a few) in the middle of the longest dry ledges, those without a pad first. */
	void AddCampfires(FUghPlacer& Placer, FRandomStream& Random);
	/** Palms (a few), the tallest that fit, apart from each other. */
	void AddPalms(FUghPlacer& Placer, FRandomStream& Random);
	/** Totems and huts of the tribe where there is room for them. */
	void AddLandmarks(FUghPlacer& Placer, FRandomStream& Random);
	/** Bushes, ferns, jungle plants, rocks, stumps and bones along every ledge. */
	void AddPlants(FUghPlacer& Placer, FRandomStream& Random);
	/** Meadows: rows of grass, patches of flowers, along every ledge from as near the slab of the play as they may. */
	void AddMeadows(FUghPlacer& Placer, FRandomStream& Random);
	/** Lianas hanging from the ceilings. */
	void AddVines(FUghPlacer& Placer, FRandomStream& Random);
	/** Lianas hanging down the cave's back wall from its ceilings, in patches. */
	void AddCreepers(FUghPlacer& Placer, FRandomStream& Random);
}
