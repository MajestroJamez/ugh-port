// The flight from stone to stone between two levels: no cut to black.
#pragma once

#include "CoreMinimal.h"
#include "UghFlight.h"
#include "UghIntro.h"

/**
 * The drone's flight to the next level's stone (Jan, step 32f): from a level done (its fireworks) or from the level
 * selection's stone chosen (FUghIsles) to the game's camera at the stone of the level that comes - no black. The stone
 * the levels are carved into (AUghSeaStack) stays where it is in the world, with the level built in it; the archipelago
 * (AUghArchipelago, FUghIsles::Layout) is placed around it so that the stone it stands for is the level's. The flight
 * goes from that stone (or the choosing camera) up over the sea to the next level's stone of the archipelago - `Shift`
 * away in the world as it is before - into a bank of mist (Mist: the picture fades into its colour, never black). In
 * the mist the world switches (the game mode, WantsSwitch .. Switched): the next level is built there (in short steps,
 * a frame each; the flight holds still meanwhile), the stone stands where that stone was - the archipelago moves by
 * -Shift and so does the camera (Pose: the way minus Shift from then on; Along is the way itself, without the jump) -,
 * then it flies out of the mist and approaches its level, braking softly into the game's camera (as FUghIntro). Its
 * curve bends smoothly, its speed rises from a stop, goes on, brakes to a stop (UghFlight); it looks at the level it
 * leaves, then at the level it goes to. A key hurries it (only once the logic is at the next level's caption: a key
 * before does not count there): before the mist into it at once, the rest of the way after it in HurrySeconds from
 * SkipTo before its end (as FUghIntro). Only the frontend's: the logic and its timing do not change (its caption waits
 * for its key meanwhile, shown over the flight).
 */
class FUghVoyage
{
public:
	/** Seconds: from a level done; from the level selection's stone; the rest when hurried; hurried into the mist. */
	static constexpr double FromPlay = 9, FromIsles = 9, HurrySeconds = 0.9, SkipTo = 1.6;
	/** Seconds: the mist thickening before its middle, thinning after the switch, thickening hurried. */
	static constexpr double MistIn = 0.5, MistOut = 0.7, MistHurried = 0.25;
	/** The flight's speed (UghFlight): rising from a stop, braking from the middle on. */
	static constexpr UghFlight::FProfile Profile{ 0.15, 0.5 };
	/** A frame longer than this moves it only this far (seconds). */
	static constexpr double MaxStep = 1.0 / 20;
	/** The stone's jungle is shown until this long before its end (the game's camera does not see it, AUghSeaStack). */
	static constexpr double JungleBeforeEnd = 0.65;

	/**
	 * From `From` (at rest: the game's camera of the level done, `bFromPlay`; or the camera choosing over the
	 * archipelago) to the game's camera at the stone `Shift` away (the world before the switch); `Stone` is where the
	 * stone of the levels stands (its foot at the sea, the world); `Game` the game's camera, the sea at `SeaZ`.
	 */
	void Start(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift, const FVector& Stone,
		bool bFromPlay, double SeaZ);
	/** On `Seconds` (holding still in the mist until Switched). */
	void Advance(double Seconds);
	/** Hurried: into the mist at once (before the switch), else the rest in HurrySeconds. */
	void Hurry();
	/** No flight (back to the menu, the play began). */
	void Stop() { bFlying = false; }

	bool IsFlying() const { return bFlying; }
	double GetTime() const { return Clock.GetTime(); }
	double GetDuration() const { return Clock.GetDuration(); }
	/** When the mist is thickest (seconds into it): the world switches there. */
	double GetSwitchTime() const { return SwitchTime; }
	/** The mist is thick: the world may switch (the next level built, the archipelago moved). */
	bool WantsSwitch() const { return bFlying && !bSwitched && Mist >= 1 && (Clock.GetTime() >= SwitchTime || bHurried); }
	/** The world has switched: on out of the mist. */
	void Switched();
	bool IsSwitched() const { return bSwitched; }
	/** It got to the game's camera (since its start). */
	bool HasArrived() const { return bArrived; }
	/** How thick the mist is, 0 .. 1 (the picture fades into its colour). */
	double GetMist() const { return FMath::SmoothStep(0.0, 1.0, Mist); }
	const FVector& GetShift() const { return Shift; }
	/** The stone's jungle is seen. */
	bool ShowsJungle() const { return bFlying && Clock.GetTime() < Clock.GetDuration() - JungleBeforeEnd; }

	/** The camera now (the world as it is: after the switch the way minus Shift) for the game's camera `Game`. */
	FUghCameraPose Pose(const FUghCameraPose& Game, double SeaZ) const;
	/** The camera now on the way itself (the world before the switch: no jump - the camera's log). */
	FUghCameraPose Along(const FUghCameraPose& Game, double SeaZ) const;
	/** The camera `Time` seconds into such a flight of `Duration` (the world before the switch): Game + Shift last. */
	static FUghCameraPose At(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift,
		const FVector& Stone, bool bFromPlay, double SeaZ, double Duration, double Time);
	/**
	 * Its way (the world before the switch): pulled back and up from the stone (from the play), high over the sea
	 * (above every stone of the archipelago) in front of the next stone - the mist -, down to it, its game's camera;
	 * the index of the mist's point.
	 */
	static TArray<FVector> Waypoints(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift,
		const FVector& Stone, bool bFromPlay, double SeaZ, int32& MistPoint);
	/** When such a flight of `Duration` passes the mist's point (seconds). */
	static double MistTime(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift,
		const FVector& Stone, bool bFromPlay, double SeaZ, double Duration);

private:
	FUghFlightClock Clock{ FromPlay, HurrySeconds, Profile };
	FUghCameraPose From;
	FVector Shift = FVector::ZeroVector, Stone = FVector::ZeroVector;
	bool bFromPlay = true;
	bool bFlying = false, bSwitched = false, bHurried = false, bArrived = false;
	double SwitchTime = 0;
	double Mist = 0;
};
