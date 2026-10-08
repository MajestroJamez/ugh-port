#include "UghIntro.h"

namespace
{
	/**
	 * The way to the end: the curve goes through these points and ends in the end's place - how far behind the end
	 * (units, along its view), how far to its right, how high above the sea (units) or, by Rise, a share of the way up
	 * to the end's height. Low over the waves, a swerve to the right and back, rising at the end.
	 */
	struct FWaypoint
	{
		double Behind, Right, Above, Rise;
	};
	const FWaypoint Waypoints[] = { { 26000, -3500, 260, 0 }, { 17000, 2200, 420, 0 }, { 9000, 1600, 300, 0 },
		{ 3500, -500, 320, 0.5 } };
	/** Never lower above the sea than this, units. */
	constexpr double LowestAbove = 150;

	/** Its altitude goes up and down this much (units) this often (a second), less and less on the way. */
	constexpr double Bob = 35, BobRate = 0.8;
	/** It looks along its way and at the level, the more at the level the nearer it is: from this share of it. */
	constexpr double LookAtFirst = 0.7;
	/**
	 * It banks like a drone into its turns: how hard a turn (units per second squared, sideways to its way) banks it most
	 * of MaxRoll, the turn taken over this many seconds around the moment (a smooth bank, not every wobble).
	 */
	constexpr double RollAccel = 5000, TurnSeconds = 0.6;
	/** Seconds apart of the positions its way is taken from. */
	constexpr double Step = 0.06;
	/** Less blur than the engine's while it flies fast (its motion blur amount). */
	constexpr float FlightBlur = 0.2f;
	/**
	 * Out in the daylight the exposure is darker by this much (EV), less and less from this share of the flight on: the
	 * mood's own is the cave's, as an eye adapts flying into it.
	 */
	constexpr float Outdoors = -0.8f;
	constexpr double AdaptFrom = 0.45;

	using UghFlight::FWay;

	/** Where it is `Time` seconds into the flight along `Way` (before its start on along the curve, after its end still there). */
	FVector Place(const FWay& Way, double Time)
	{
		const double S = UghFlight::Share(FMath::Min(Time / FUghIntro::Duration, 1.0), FUghIntro::Profile);
		return Way.Along(S) + FVector(0, 0, Bob * FMath::Sin(UE_TWO_PI * BobRate * Time) * FMath::Cube(1 - S));
	}

	/** Which way it goes and how it turns (its acceleration) at `Time`, from positions `Apart` seconds around it. */
	void Motion(const FWay& Way, double Time, double Apart, FVector& Velocity, FVector& Accel)
	{
		const FVector Before = Place(Way, Time - Apart), Now = Place(Way, Time), After = Place(Way, Time + Apart);
		Velocity = (After - Before) / (2 * Apart);
		Accel = (Before + After - 2 * Now) / (Apart * Apart);
	}
}

TArray<FVector> FUghIntro::Waypoints(const FUghCameraPose& End, double SeaZ)
{
	const FRotator Heading(0, End.Rotation.Yaw, 0);
	const FVector Forward = Heading.Vector(), Right = FRotationMatrix(Heading).GetUnitAxis(EAxis::Y);
	TArray<FVector> Points;
	for (const FWaypoint& Point : ::Waypoints)
	{
		FVector At = End.Location - Forward * Point.Behind + Right * Point.Right;
		At.Z = FMath::Max(FMath::Lerp(SeaZ + Point.Above, End.Location.Z, Point.Rise), SeaZ + LowestAbove);
		Points.Add(At);
	}
	Points.Add(End.Location);
	return Points;
}

void FUghIntro::Start()
{
	Clock.Start();
	Faded = 0;
	Settled = 0;
	Held = 0;
}

void FUghIntro::Advance(double Seconds)
{
	if (!IsFlying())
	{
		return;
	}
	if (Settled < SettleFrames)
	{
		// (a long frame - the GPU catching up on the new level - does not count, but it waits no longer than the limit)
		++Held;
		Settled += Seconds <= MaxStep || Held >= SettleLimit ? 1 : 0;
		return;
	}
	Faded += Seconds;
	// hurried: on the real clock (it must end before the play)
	Clock.Advance(Seconds, MaxStep);
}

void FUghIntro::Hurry()
{
	if (!IsFlying() || Clock.IsHurried())
	{
		return;
	}
	if (Shown() == 0)
	{
		// nothing seen yet: the flight starts nearer instead of rushing the whole of it
		Clock.SkipTo(Duration - SkipTo);
	}
	Clock.Hurry();
}

FUghCameraPose FUghIntro::At(const FUghCameraPose& End, double SeaZ, double Time)
{
	if (Time >= Duration)
	{
		return End;
	}
	const double U = FMath::Max(Time, 0.0) / Duration;
	const FWay Way(Waypoints(End, SeaZ));
	const FVector Location = Place(Way, Time);
	FVector Velocity, Accel;
	Motion(Way, Time, Step, Velocity, Accel);
	// the level: where the end's view meets the plane of the play (world y 0, UghShapes)
	const FVector View = End.Rotation.Vector();
	const double Reach = FMath::Abs(View.Y) > 0.1 ? -End.Location.Y / View.Y : 7000;
	const FVector Level = End.Location + View * Reach;
	const double LookAt = FMath::Lerp(LookAtFirst, 1.0, FMath::SmoothStep(0.1, 0.85, U));
	const FVector Look = FMath::Lerp(Velocity.GetSafeNormal(), (Level - Location).GetSafeNormal(), LookAt);

	FUghCameraPose Pose;
	Pose.Location = Location;
	Pose.Rotation = Look.GetSafeNormal().Rotation();
	// banking into the turn, sideways to its way (a positive roll lowers the right; braking does not bank it), the turn
	// over TurnSeconds; level again as it stops
	FVector Heading, Turn;
	Motion(Way, Time, TurnSeconds / 2, Heading, Turn);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Heading.GetSafeNormal2D());
	const double Sideways = Turn | Side;
	Pose.Rotation.Roll = MaxRoll * FMath::Tanh(Sideways / RollAccel) * (1 - FMath::SmoothStep(0.45, 0.85, U));
	Pose.FieldOfView = FMath::Lerp(StartFieldOfView, End.FieldOfView, float(FMath::SmoothStep(0.35, 1.0, U)));
	Pose.MotionBlur = FlightBlur;
	Pose.ExposureBias = Outdoors * float(1 - FMath::SmoothStep(AdaptFrom, 1.0, U));
	return Pose;
}
