#include "UghIntro.h"

#include "Algo/BinarySearch.h"

namespace
{
	/**
	 * The way to the end: the curve (Catmull-Rom) goes through these points and ends in the end's place - how far
	 * behind the end (units, along its view), how far to its right, how high above the sea (units) or, by Rise, a
	 * share of the way up to the end's height. Low over the waves, a swerve to the right and back, rising at the end.
	 */
	struct FWaypoint
	{
		double Behind, Right, Above, Rise;
	};
	const FWaypoint Waypoints[] = { { 26000, -3500, 260, 0 }, { 17000, 2200, 420, 0 }, { 9000, 1600, 300, 0 },
		{ 3500, -500, 320, 0.5 } };
	/** Never lower above the sea than this, units. */
	constexpr double LowestAbove = 150;
	/** The curve sampled this many times a piece between two points (how far along it each is). */
	constexpr int32 SamplesAPiece = 64;

	/** The share of the flight at full speed; then it brakes, softer and softer, to a stop. */
	constexpr double Cruise = 0.5;
	/** Its altitude goes up and down this much (units) this often (a second), less and less on the way. */
	constexpr double Bob = 35, BobRate = 0.8;
	/** It looks along its way and at the level, the more at the level the nearer it is: from this share of it. */
	constexpr double LookAtFirst = 0.7;
	/** How hard a turn (units per second squared sideways) banks it most of MaxRoll. */
	constexpr double RollAccel = 2500;
	/** Seconds apart of the positions its way and its turns are taken from. */
	constexpr double Step = 0.06;
	/** Less blur than the engine's while it flies fast (its motion blur amount). */
	constexpr float FlightBlur = 0.2f;
	/**
	 * Out in the daylight the exposure is darker by this much (EV), less and less from this share of the flight on: the
	 * mood's own is the cave's, as an eye adapts flying into it.
	 */
	constexpr float Outdoors = -0.8f;
	constexpr double AdaptFrom = 0.45;

	/** The points of the curve: its waypoints and the end, the world. */
	TArray<FVector> Through(const FUghCameraPose& End, double SeaZ)
	{
		const FRotator Heading(0, End.Rotation.Yaw, 0);
		const FVector Forward = Heading.Vector(), Right = FRotationMatrix(Heading).GetUnitAxis(EAxis::Y);
		TArray<FVector> Points;
		for (const FWaypoint& Point : Waypoints)
		{
			FVector At = End.Location - Forward * Point.Behind + Right * Point.Right;
			At.Z = FMath::Max(FMath::Lerp(SeaZ + Point.Above, End.Location.Z, Point.Rise), SeaZ + LowestAbove);
			Points.Add(At);
		}
		Points.Add(End.Location);
		return Points;
	}

	/** The curve at C (0 at its first point, 1 at the next ...), going on straight beyond its ends. */
	FVector Curve(const TArray<FVector>& Points, double C)
	{
		const int32 Last = Points.Num() - 1;
		const int32 I = FMath::Clamp(FMath::FloorToInt32(C), 0, Last - 1);
		const double T = FMath::Clamp(C - I, 0.0, 1.0);
		auto Point = [&](int32 Index)
		{
			return Index < 0 ? 2 * Points[0] - Points[1]
				: Index > Last ? 2 * Points[Last] - Points[Last - 1] : Points[Index];
		};
		const FVector P0 = Point(I - 1), P1 = Point(I), P2 = Point(I + 1), P3 = Point(I + 2);
		return 0.5 * (2 * P1 + (P2 - P0) * T + (2 * P0 - 5 * P1 + 4 * P2 - P3) * T * T +
			(3 * P1 - P0 - 3 * P2 + P3) * T * T * T);
	}

	/** The curve measured: how far along it (units) each of its samples is. */
	struct FMeasured
	{
		TArray<FVector> Points;
		TArray<double> Lengths;

		FMeasured(const FUghCameraPose& End, double SeaZ) : Points(Through(End, SeaZ))
		{
			const int32 Samples = (Points.Num() - 1) * SamplesAPiece;
			Lengths.Add(0);
			FVector Previous = Points[0];
			for (int32 Sample = 1; Sample <= Samples; ++Sample)
			{
				const FVector Next = Curve(Points, double(Sample) / SamplesAPiece);
				Lengths.Add(Lengths.Last() + FVector::Dist(Previous, Next));
				Previous = Next;
			}
		}

		/** The point a share `S` (0 .. 1) of the curve's length along it. */
		FVector Along(double S) const
		{
			const double Wanted = FMath::Clamp(S, 0.0, 1.0) * Lengths.Last();
			const int32 Upper = FMath::Clamp(Algo::LowerBound(Lengths, Wanted), 1, Lengths.Num() - 1);
			const double Part = (Wanted - Lengths[Upper - 1]) / FMath::Max(Lengths[Upper] - Lengths[Upper - 1], 1e-6);
			return Curve(Points, (Upper - 1 + Part) / SamplesAPiece);
		}
	};

	/** How much of the way is behind it at `U` (0 .. 1 of the flight): at full speed, then braking to a stop. */
	double Share(double U)
	{
		const double Speed = 1 / (Cruise + (1 - Cruise) / 3);
		if (U < Cruise)
		{
			return Speed * U;
		}
		const double W = FMath::Min((U - Cruise) / (1 - Cruise), 1.0);
		return Speed * (Cruise + (1 - Cruise) * (1 - FMath::Cube(1 - W)) / 3);
	}

	/** Where it is `Time` seconds into the flight along `Way`. */
	FVector Place(const FMeasured& Way, double Time)
	{
		const double U = Time / FUghIntro::Duration;
		const double S = Share(U);
		return Way.Along(S) + FVector(0, 0, Bob * FMath::Sin(UE_TWO_PI * BobRate * Time) * (1 - S));
	}
}

void FUghIntro::Start()
{
	Time = 0;
	Rate = 1;
	Settled = 0;
	bFlying = true;
}

void FUghIntro::Advance(double Seconds)
{
	if (!bFlying || Settled++ < SettleFrames)
	{
		return;
	}
	Time = FMath::Min(Time + FMath::Min(Seconds, MaxStep) * Rate, Duration);
	bFlying = Time < Duration;
}

void FUghIntro::Hurry()
{
	if (bFlying)
	{
		Rate = FMath::Max(Rate, (Duration - Time) / HurrySeconds);
	}
}

FUghCameraPose FUghIntro::At(const FUghCameraPose& End, double SeaZ, double Time)
{
	if (Time >= Duration)
	{
		return End;
	}
	const double U = FMath::Max(Time, 0.0) / Duration;
	const FMeasured Way(End, SeaZ);
	const FVector Location = Place(Way, Time);
	// its way and its turn (its acceleration), from a moment around it within the flight
	const double Around = FMath::Clamp(Time, Step, Duration - Step);
	const FVector Before = Place(Way, Around - Step), After = Place(Way, Around + Step);
	const FVector Accel = (Before + After - 2 * Place(Way, Around)) / (Step * Step);
	// the level: where the end's view meets the plane of the play (world y 0, UghShapes)
	const FVector View = End.Rotation.Vector();
	const double Reach = FMath::Abs(View.Y) > 0.1 ? -End.Location.Y / View.Y : 7000;
	const FVector Level = End.Location + View * Reach;
	const double LookAt = FMath::Lerp(LookAtFirst, 1.0, FMath::SmoothStep(0.1, 0.85, U));
	const FVector Look = FMath::Lerp((After - Before).GetSafeNormal(), (Level - Location).GetSafeNormal(), LookAt);

	FUghCameraPose Pose;
	Pose.Location = Location;
	Pose.Rotation = Look.GetSafeNormal().Rotation();
	// banking into the turn (a positive roll lowers the right), level again as it stops
	const double Sideways = Accel | FRotationMatrix(Pose.Rotation).GetUnitAxis(EAxis::Y);
	Pose.Rotation.Roll = MaxRoll * FMath::Tanh(Sideways / RollAccel) * (1 - FMath::SmoothStep(0.55, 0.9, U));
	Pose.FieldOfView = FMath::Lerp(StartFieldOfView, End.FieldOfView, float(FMath::SmoothStep(0.35, 1.0, U)));
	Pose.MotionBlur = FlightBlur;
	Pose.ExposureBias = Outdoors * float(1 - FMath::SmoothStep(AdaptFrom, 1.0, U));
	return Pose;
}
