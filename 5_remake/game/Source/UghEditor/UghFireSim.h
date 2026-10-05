// A fire simulated as a gas, offline: the flames of the flipbooks.
#pragma once

#include "CoreMinimal.h"
#include "UghFlipbook.h"

/** Where and how a simulated fire is fed (sizes a part of the grid's width). */
struct FUghFireSource
{
	/** How wide its bed of fuel is, how high above the floor it begins, how high it is. */
	double Width, Lift, Height;
	/** About how many tongues it feeds across its width (its fuel comes and goes by noise along it). */
	double Tongues;
	/** How hard the hot gas rises, how much the flame swirls (vorticity, noise), how fast its heat fades (1/s). */
	double Buoyancy, Swirl, Turbulence, Cooling;
};

/**
 * A flame in a vertical slice of air (Stam's stable fluids on a staggered grid of Width x Height cells, open at the top
 * and the sides, a floor below): fuel fed at its source by noise burns where it is hot, turning into heat and a little
 * soot; the heat rises (buoyancy), the swirls are kept alive (vorticity confinement) and stirred by rising noise,
 * everything is carried by the flow (semi-Lagrangian, the heat and fuel by MacCormack: sharp tongues) and the flow
 * kept free of divergence (a pressure solve); the heat cools. The same seed, the same flame.
 */
class FUghFireSim
{
public:
	FUghFireSim(int32 InWidth, int32 InHeight, const FUghFireSource& InSource, int32 Seed);

	/** Simulates `Seconds` more (in steps of at most MaxStep). */
	void Advance(double Seconds);
	/** The light the flame gives off now (linear, top row first): blackbody colours of its heat, dimmed by soot. */
	UghFlipbook::FFrame Light() const;

private:
	static constexpr double MaxStep = 1.0 / 60;

	void Step(double Dt);
	void Feed(double Dt);
	void Burn(double Dt);
	void Push(double Dt);
	void Project();

	float Sample(const TArray<float>& Field, int32 NX, int32 NY, float OX, float OY, float X, float Y) const;
	FVector2f Velocity(float X, float Y) const;
	/** `Field` (NX x NY values at cell positions + OX, OY) carried back along the flow for `Dt`. */
	TArray<float> Carried(const TArray<float>& Field, int32 NX, int32 NY, float OX, float OY, float Dt) const;
	/** The same, MacCormack: corrected by carrying it back, clamped to its neighbours (no new extremes). */
	TArray<float> CarriedSharp(const TArray<float>& Field, float Dt) const;

	const int32 Width, Height;
	const FUghFireSource Source;
	const double Seed;
	double Time = 0;
	/** The flow on the cells' faces (cells per second): U across ((Width + 1) x Height), V up (Width x (Height + 1)). */
	TArray<float> U, V;
	/** In the cells: heat (0 .. about 1), fuel, soot, pressure. */
	TArray<float> Heat, Fuel, Soot, Pressure;
};
