// A copter falling into the sea: its splash, how it bobs up again.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/** A splash where a copter falling into the sea meets it, or the foam where it comes up again (FUghDunks). */
struct FUghDunkSplash
{
	int32 Player = 0;
	FVector2D Place = FVector2D::ZeroVector;   // pixels: the middle of its body on the surface
	double Scale = 1;
	bool bSurfacing = false;   // it came up again (foam), not fell in
};

/** How a copter afloat is seen besides where the logic has it (FUghDunks::Of). */
struct FUghCopterBob
{
	double Lift = 0;   // pixels up
	double Roll = 0;   // radians about the view's axis (positive: its right side up)
};

/**
 * The copters falling into the sea. In the logic (CopterPhysics) the water is no crash and costs no life: a copter
 * whose waterline (Waterline below its top) goes under the surface is braked hard (WATER_BRAKE: a fast fall stops in
 * about a third of a second, some 20 px down), cannot pedal there and floats up (BUOYANCY, faster and faster, about a
 * second) until its waterline is at the surface, where it stops dead and floats (its top at the water row - Waterline;
 * it may take off again; a passenger in the water may swim to it). Only a hard bounce off the rock - the sea's bottom
 * too - crashes it (the explosion, with a splash at the water: FUghEffectPlayer).
 *
 * Seen here: where its waterline meets the surface falling at least MinSpeed, a big splash (the burst dunk: a column
 * and a crown of water, drops, foam, rings; bigger the faster), the surface churning there for a while (Stirs); under
 * the water it is where the logic has it; where it comes up and stops it bobs on (the speed it came up with carried on,
 * at most MaxBob pixels, dying away in BobSeconds) with foam around it, and it rocks after the splash. The bob and the
 * rock end when the logic takes it off the surface. Only decoration: nothing goes back to the logic. Test Ugh.Dunk
 * (UghDunkTests.cpp).
 */
class FUghDunks
{
public:
	/** From a copter's top to its waterline, pixels (the logic's CopterShape::WATERLINE). */
	static constexpr int32 Waterline = 18;
	/** A copter going down slower than this (pixels a step) only goes in (the rings of the water). */
	static constexpr double MinSpeed = 0.35;
	/** The splash's size by the speed it falls in at (pixels a step): 1 at FullSpeed, within these. */
	static constexpr double FullSpeed = 1.2, MinScale = 0.6, MaxScale = 1.6;
	/** Coming up faster than this (pixels a step) it foams (the burst boil this big). */
	static constexpr double MinFoamRise = 0.15, FoamScale = 0.8;
	/** The bob: at most this high (pixels), this long a swing, dying away in this (seconds). */
	static constexpr double MaxBob = 1.6, BobPeriod = 0.9, BobSeconds = 0.7;
	/** The rock after a splash: at most this much (radians, at FullSpeed), this long a swing, dying away in this. */
	static constexpr double MaxRoll = 0.09, RollPeriod = 1.2, RollSeconds = 0.9;
	/** Taken off the surface, the bob and the rock end in this (seconds). */
	static constexpr double EndSeconds = 0.15;
	/** The water churns where it fell in this long (seconds), this much at first (UghWater::Rings' strength). */
	static constexpr double StirSeconds = 3, StirStrength = 5;

	/**
	 * A frame between `Previous` and `Current` (Alpha), `Seconds` after the last: a copter's waterline reaching the
	 * surface splashes, one coming up to it foams and bobs.
	 */
	void Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds);
	/** The splashes since the last call. */
	TArray<FUghDunkSplash> TakeSplashes();
	/** How copter `Player` is seen besides where the logic has it. */
	FUghCopterBob Of(int32 Player) const;
	/** The water churning where copters fell in: its place on the surface (world) and strength (UghWater::Rings). */
	TArray<FVector4> Stirs(double Surface) const;
	/** Seconds since a copter last fell into the water (a shot of it), none before. */
	TOptional<double> Age() const;
	/** Forgets everything (a new level, an attempt, the menu). */
	void Reset();

	/** How deep the logic has a copter whose top is at `Y` (1/32 px) under the water at `WaterLevel` (1/32 px). */
	static int32 DepthOf(int32 Y, int32 WaterLevel);

private:
	struct FCopter
	{
		bool bSeen = false;        // a frame of it in the play before
		double Waterline = 0;      // seen in the last frame (pixels)
		double Surface = 0;        // then
		int32 LastY = 0;           // the logic's in the last frame (1/32 px)
		int32 LastDepth = 0;       // the logic's then (pixels)
		double Rise = 0;           // pixels a step it rose under the water lately
		double Fell = -1;          // seconds since it fell in, -1 never
		double FellScale = 0;
		FVector2D FellAt = FVector2D::ZeroVector;   // pixels
		double BobTime = 0, Bob = 0;     // seconds since it came up, its height (pixels)
		double RollTime = 0, Roll = 0;   // seconds since it splashed or came up, its rock (radians)
		double Hold = 0;           // 1 afloat, going to 0 when taken off
	};

	FCopter Copters[2];
	TArray<FUghDunkSplash> Splashes;
	double Since = -1;   // seconds since the last fall into the water, -1 none
};
