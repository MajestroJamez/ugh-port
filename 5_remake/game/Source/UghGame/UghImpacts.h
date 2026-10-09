// The feel of an impact: the camera shaken a little, the pilot's gamepad rumbling.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

/** What hit what (FUghImpacts::FeelOf: how hard it is felt). */
enum class EUghImpact : uint8
{
	Crash,      // a copter crashed (the logic's event)
	StoneHit,   // a dropped stone knocked out an enemy or bounced off the tree (the logic's events)
	Dunk,       // a copter fell into the sea (FUghDunks; no event, no crash)
	Edge,       // a copter bumped into the left, right or top edge of the screen (AUghFringe; no event)
	Count
};

/** How an impact is felt at its full strength. */
struct FUghImpactFeel
{
	double Shake;      // the camera's swing, units (a pixel of the screen is 10; the camera is 113 m away)
	double Frequency;  // its shakes a second
	double Damping;    // seconds in which it dies to 1/e
	float Rumble;      // the gamepad's motors, 0 .. 1 (the large one; the small one a little less)
	double Seconds;    // how long the gamepad rumbles, dying away
};

/** A gamepad's motors now (FUghImpacts::Rumble). */
struct FUghRumble
{
	float Large = 0, Small = 0;
	bool IsOff() const { return Large <= 0 && Small <= 0; }
	bool operator==(const FUghRumble& Other) const = default;
};

/**
 * The feel of the impacts (only decoration, the logic does not know it): a crash, a dropped stone hitting an enemy (the
 * pilot who dropped it), a copter falling into the sea, a bump into the screen's edge. Each shakes the camera a little
 * (a damped swing in the screen's plane, the stronger the harder, never more than MostShake - the play stays readable;
 * none while the shake is off, FUghSettings::bShake) and rumbles the gamepad of the pilot it happened to (the large
 * motor and the small one, dying away). Pure: the game mode moves the camera (Offset) and the pads (FUghPads). Test
 * Ugh.Impact.* (UghImpactTests.cpp).
 */
class FUghImpacts
{
public:
	/** The camera never swings further (units: 1.6 pixels of the screen). */
	static constexpr double MostShake = 16;
	/** The pilots (and their gamepads). */
	static constexpr int32 Pilots = 2;

	static const FUghImpactFeel& FeelOf(EUghImpact Impact);

	/** The camera shaken or not (the setting); off stops a shake at once. */
	void SetShake(bool bOn);
	bool IsShaking() const { return !Shakes.IsEmpty(); }
	/** Nothing goes on (a level's end, the menu). */
	void Reset();

	/** An event of the logic (after the step that made it): a crash, a dropped stone hitting an enemy. */
	void OnEvent(const ugh_logic_event& Event);
	/** Copter `Player` fell into the sea (FUghDunkSplash: `Scale` 1 at the speed of a usual fall). */
	void OnDunk(int32 Player, double Scale);
	/** Copter `Player` bumped into an edge at `Speed` pixels a second, `bTop` the top one (AUghFringe). */
	void OnBump(int32 Player, double Speed, bool bTop);
	/**
	 * `Impact` to `Player` (INDEX_NONE: every pilot) at `Strength` (1 its feel, FeelOf), shaking along `Along` (the
	 * screen's right and up).
	 */
	void Add(EUghImpact Impact, int32 Player, double Strength, const FVector2D& Along = FVector2D(0.6, 0.8));

	/** `Seconds` on. */
	void Advance(double Seconds);
	/** The camera's offset now: units to the screen's right and up. */
	FVector2D Offset() const;
	/** Pilot `Player`'s gamepad now. */
	FUghRumble Rumble(int32 Player) const;
	/** The impacts so far (a test). */
	int32 Count(EUghImpact Impact) const { return Counts[int32(Impact)]; }

private:
	struct FShake
	{
		double Amplitude;   // units
		double Frequency, Damping;
		FVector2D Along;
		double Age = 0;
	};
	struct FRumbling
	{
		float Strength;
		double Seconds;
		double Age = 0;
	};

	bool bShake = true;
	TArray<FShake> Shakes;
	TArray<FRumbling> Rumbles[Pilots];
	int32 Counts[int32(EUghImpact::Count)] = {};
	int32 Dropper = INDEX_NONE;   // the pilot who last let a passenger (the stone) fall
};
