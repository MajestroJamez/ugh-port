// Stalactites and fallen rocks of a level's cave.
#pragma once

#include "CoreMinimal.h"

struct ugh_logic;

/** A shape added to the rock behind the slab of the play (pixels; depth in pixels behind the plane of the play). */
struct FUghRockStamp
{
	enum class EKind : uint8
	{
		/** A cone hanging down from Centre (in its ceiling), Radius at the top, Length long. */
		Stalactite,
		/** A ball of Radius around Centre, lying on a floor. */
		Boulder
	};

	EKind Kind = EKind::Boulder;
	/** x, y of the screen and the depth. */
	FVector Centre = FVector::ZeroVector;
	double Radius = 0, Length = 0;
};

/**
 * Where the cave of the level being played has stalactites (under flat ceilings with room below them) and fallen
 * rocks (at the feet of its walls, a few on its floors, none on a pad), always behind the slab of the play so that
 * they never touch it; the same level always gets the same ones (the places themselves seed the choice).
 */
namespace UghRockFeatures
{
	TArray<FUghRockStamp> Plan(const ugh_logic* Logic);
}
