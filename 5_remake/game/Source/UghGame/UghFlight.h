// What the camera's flights share: a smooth curve, a speed that changes smoothly, a clock a key hurries.
#pragma once

#include "CoreMinimal.h"

/**
 * The camera's flights (the flight to the stone at a level's start, FUghIntro; the flights over the archipelago of the
 * level selection, FUghIsles): a way along a smooth curve, the share of it behind the camera at a share of the flight's
 * time - its speed rising smoothly from a stop (Rise), at full speed, then braking to a stop (Cruise .. 1), so that the
 * speed, its change and the change of that are continuous throughout - and a clock that a key hurries to its end along
 * the smoothest way from the speed it has to a stop.
 */
namespace UghFlight
{
	/**
	 * How the speed changes over a flight (shares of its time): from 0 up to full speed until Rise (0: at full speed
	 * from the start, as from black), full speed until Cruise, then braking to a stop at its end (Cruise 1: none, it
	 * ends at full speed).
	 */
	struct FProfile
	{
		double Rise = 0, Cruise = 0.35;
	};

	/** How much of the way is behind it at `U` (0 .. 1 of the flight). */
	double Share(double U, const FProfile& Profile);
	/** How fast Share grows at `U` (its first and second derivatives by U). */
	void ShareRates(double U, const FProfile& Profile, double& Rate, double& Change);
	/** The U (0 .. 1) whose Share is `S`, from `From` on. */
	double ShareAt(double S, double From, const FProfile& Profile);

	/**
	 * A curve through points (the world): a natural cubic spline over their distances apart, so that it bends smoothly
	 * through them (its curvature, and so the turns a camera banks into, has no jumps at the points as a Catmull-Rom
	 * curve's has), measured along its length.
	 */
	struct FWay
	{
		TArray<FVector> Points;
		TArray<double> Knots;      // the parameter at each point (the distances between them added up)
		TArray<FVector> Bends;     // the second derivative at each point
		TArray<double> Lengths;    // how far along the curve each of its samples is

		explicit FWay(const TArray<FVector>& InPoints);
		/** The curve at parameter T (0 at its first point, Knots.Last() at the end). */
		FVector At(double T) const;
		/** The point a share `S` (0 .. 1) of the curve's length along it (before its start on along its first piece). */
		FVector Along(double S) const;
		/** How long it is (units). */
		double Length() const { return Lengths.Last(); }
	};
}

/**
 * The clock of a flight `Duration` seconds long (UghFlight::FProfile): its time goes on with the real one (at most a
 * step a frame: a long frame does not jump it); a key hurries the rest of it into `HurrySeconds` - from the share of
 * the way, the speed and the acceleration it has then, the smoothest way (a quintic of the share of that time) to its
 * end with no speed or deceleration left; near its end, where that would overshoot, a clock that speeds up smoothly
 * from its pace.
 */
class FUghFlightClock
{
public:
	FUghFlightClock(double InDuration, double InHurrySeconds, const UghFlight::FProfile& InProfile)
		: Duration(InDuration), HurrySeconds(InHurrySeconds), Profile(InProfile) {}

	/** From the start (its length may change: `NewDuration`, else as it was). */
	void Start(double NewDuration = 0);
	/** On `Seconds` (when not hurried at most `MaxStep`); it ends at its end. */
	void Advance(double Seconds, double MaxStep = 1e9);
	/** The rest in HurrySeconds (nothing when it ends in time as it is). */
	void Hurry();
	/** Jumps on to `At` seconds (not back). */
	void SkipTo(double At) { Time = FMath::Clamp(FMath::Max(Time, At), 0.0, Duration); }
	void Stop() { bRunning = false; }

	bool IsRunning() const { return bRunning; }
	bool IsHurried() const { return bHurried; }
	double GetTime() const { return Time; }
	double GetDuration() const { return Duration; }
	/** The share of its time, 0 .. 1, and of its way. */
	double GetU() const { return Duration > 0 ? FMath::Clamp(Time / Duration, 0.0, 1.0) : 1.0; }
	double GetShare() const { return UghFlight::Share(GetU(), Profile); }
	const UghFlight::FProfile& GetProfile() const { return Profile; }

private:
	double Duration, HurrySeconds;
	UghFlight::FProfile Profile;
	double Time = 0;
	bool bRunning = false;
	bool bHurried = false;
	double Hurried = 0, HurryFrom = 0;   // seconds since the key, the flight's time then
	/** The rest of the way after the key: the coefficients of its quintic (bHurryAlong), else of its clock. */
	double Hurry0 = 0, Hurry1 = 0, Hurry2 = 0, Hurry3 = 0, Hurry4 = 0, Hurry5 = 0;
	bool bHurryAlong = false;
};
