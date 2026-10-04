// How far a copter's rotor and its pilot's pedals have turned.
#pragma once

#include "CoreMinimal.h"

/**
 * The logic says only which of its sprites a copter's rotor shows (it moves to the next one the faster, the harder
 * the pilot pedals). The rotor of the model turns smoothly instead: at the speed its sprites have been changing
 * lately (smoothed over SmoothSeconds), SpritesPerTurn sprites a turn of the rotor; the pilot's crank turns
 * RotorTurnsPerPedal times slower. Without sprites changing it slows down to a stop.
 */
class FUghRotorSpin
{
public:
	static constexpr double SpritesPerTurn = 6, RotorTurnsPerPedal = 3, SmoothSeconds = 0.3;

	/** A frame `Seconds` after the last one, the rotor showing `Sprite`. */
	void Update(int32 Sprite, double Seconds);

	/** The rotor's angle, turns (0 .. 1). */
	double RotorTurn() const { return FMath::Frac(Turns); }
	/** The crank's angle, turns (0 .. 1). */
	double PedalTurn() const { return FMath::Frac(Turns / RotorTurnsPerPedal); }
	/** Rotor turns a second. */
	double Speed() const { return SpritesPerSecond / SpritesPerTurn; }

private:
	int32 LastSprite = -1;
	double SpritesPerSecond = 0;
	double Turns = 0;   // of the rotor, kept below RotorTurnsPerPedal
};
