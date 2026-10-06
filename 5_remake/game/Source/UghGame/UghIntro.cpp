#include "UghIntro.h"

#include "Algo/BinarySearch.h"

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
	/** The curve sampled this many times (how far along it each is). */
	constexpr int32 Samples = 4096;

	/**
	 * The share of the flight at full speed; then it brakes to a stop, its deceleration growing and fading smoothly
	 * (no jolt when it begins, none when it stops: speed, deceleration and its change all reach 0 at the end).
	 */
	constexpr double Cruise = 0.35;
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

	/**
	 * The curve through the points: a natural cubic spline over their distances apart, so that it bends smoothly
	 * through them (its curvature, and so the turns the camera banks into, has no jumps at the points as a Catmull-Rom
	 * curve's has), measured along its length.
	 */
	struct FWay
	{
		TArray<FVector> Points;
		TArray<double> Knots;      // the parameter at each point (the distances between them added up)
		TArray<FVector> Bends;     // the second derivative at each point
		TArray<double> Lengths;    // how far along the curve each of its Samples is

		FWay(const FUghCameraPose& End, double SeaZ) : Points(Through(End, SeaZ))
		{
			const int32 Count = Points.Num();
			Knots.Add(0);
			for (int32 I = 1; I < Count; ++I)
			{
				Knots.Add(Knots.Last() + FVector::Dist(Points[I - 1], Points[I]));
			}
			// the tridiagonal system of the natural spline (no bend at either end), solved by elimination
			Bends.Init(FVector::ZeroVector, Count);
			TArray<double> Diagonal, Upper;
			TArray<FVector> Right;
			Diagonal.Init(1, Count);
			Upper.Init(0, Count);
			Right.Init(FVector::ZeroVector, Count);
			for (int32 I = 1; I < Count - 1; ++I)
			{
				const double H0 = Knots[I] - Knots[I - 1], H1 = Knots[I + 1] - Knots[I];
				const FVector Wanted =
					6 * ((Points[I + 1] - Points[I]) / H1 - (Points[I] - Points[I - 1]) / H0);
				// eliminate the lower diagonal (H0) with the row before
				const double Pivot = 2 * (H0 + H1) - H0 * Upper[I - 1];
				Diagonal[I] = Pivot;
				Upper[I] = H1 / Pivot;
				Right[I] = (Wanted - H0 * Right[I - 1]) / Pivot;
			}
			for (int32 I = Count - 2; I >= 1; --I)
			{
				Bends[I] = Right[I] - Upper[I] * Bends[I + 1];
			}
			Lengths.Add(0);
			FVector Previous = Points[0];
			for (int32 Sample = 1; Sample <= Samples; ++Sample)
			{
				const FVector Next = At(Knots.Last() * Sample / Samples);
				Lengths.Add(Lengths.Last() + FVector::Dist(Previous, Next));
				Previous = Next;
			}
		}

		/** The curve at parameter T (0 at its first point, Knots.Last() at the end). */
		FVector At(double T) const
		{
			const int32 I = FMath::Clamp(Algo::UpperBound(Knots, T) - 1, 0, Knots.Num() - 2);
			const double H = Knots[I + 1] - Knots[I];
			const double A = (Knots[I + 1] - T) / H, B = (T - Knots[I]) / H;
			return A * Points[I] + B * Points[I + 1] +
				((A * A * A - A) * Bends[I] + (B * B * B - B) * Bends[I + 1]) * (H * H / 6);
		}

		/** The point a share `S` (0 .. 1) of the curve's length along it. */
		FVector Along(double S) const
		{
			if (S < 0)
			{
				return At(S * Knots.Last());   // before its start: on along its first piece
			}
			const double Wanted = FMath::Min(S, 1.0) * Lengths.Last();
			const int32 Upper = FMath::Clamp(Algo::LowerBound(Lengths, Wanted), 1, Lengths.Num() - 1);
			const double Part = (Wanted - Lengths[Upper - 1]) / FMath::Max(Lengths[Upper] - Lengths[Upper - 1], 1e-6);
			return At(Knots.Last() * (Upper - 1 + Part) / Samples);
		}
	};

	/** Its speed at full speed: a share of the way in a share of the flight. */
	constexpr double FullSpeed = 1 / (Cruise + (1 - Cruise) / 2);

	/**
	 * How much of the way is behind it at `U` (0 .. 1 of the flight): at full speed, then braking to a stop - its speed
	 * falls as 1 - smootherstep, so that the speed, its change and the change of that are continuous throughout.
	 */
	double Share(double U)
	{
		if (U < Cruise)
		{
			return FullSpeed * U;
		}
		const double W = FMath::Min((U - Cruise) / (1 - Cruise), 1.0);
		// the integral of 1 - (6 w^5 - 15 w^4 + 10 w^3)
		const double Braked = W - 2.5 * FMath::Pow(W, 4) + 3 * FMath::Pow(W, 5) - FMath::Pow(W, 6);
		return FullSpeed * (Cruise + (1 - Cruise) * Braked);
	}

	/** How fast Share grows at `U` (its first and second derivatives by U). */
	void ShareRates(double U, double& Rate, double& Change)
	{
		if (U < Cruise)
		{
			Rate = FullSpeed;
			Change = 0;
			return;
		}
		const double W = FMath::Min((U - Cruise) / (1 - Cruise), 1.0);
		Rate = FullSpeed * (1 - (6 * FMath::Pow(W, 5) - 15 * FMath::Pow(W, 4) + 10 * FMath::Cube(W)));
		Change = -FullSpeed * 30 * FMath::Square(W) * FMath::Square(1 - W) / (1 - Cruise);
	}

	/** The U (0 .. 1) whose Share is `S`, from `From` on. */
	double ShareAt(double S, double From)
	{
		double Low = From, High = 1;
		for (int32 Halving = 0; Halving < 48; ++Halving)
		{
			const double Middle = (Low + High) / 2;
			(Share(Middle) < S ? Low : High) = Middle;
		}
		return (Low + High) / 2;
	}

	/** Where it is `Time` seconds into the flight along `Way` (before its start on along the curve, after its end still there). */
	FVector Place(const FWay& Way, double Time)
	{
		const double S = Share(FMath::Min(Time / FUghIntro::Duration, 1.0));
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

void FUghIntro::Start()
{
	Time = 0;
	Faded = 0;
	Settled = 0;
	bFlying = true;
	bHurried = false;
}

void FUghIntro::Advance(double Seconds)
{
	if (!bFlying || Settled++ < SettleFrames)
	{
		return;
	}
	Faded += Seconds;
	if (bHurried)
	{
		// the rest of the way planned when the key came (Hurry), on the real clock: it must end before the play
		Hurried = FMath::Min(Hurried + Seconds, HurrySeconds);
		const double R = Hurried / HurrySeconds;
		if (Hurried >= HurrySeconds)
		{
			Time = Duration;
		}
		else if (bHurryAlong)
		{
			const double S = Hurry0 + R * (Hurry1 + R * (Hurry2 + R * (Hurry3 + R * (Hurry4 + R * Hurry5))));
			Time = FMath::Max(Time, Duration * ShareAt(FMath::Min(S, 1.0), HurryFrom / Duration));
		}
		else
		{
			Time = HurryFrom + Hurried + Hurry3 * FMath::Cube(Hurried);
		}
		Time = FMath::Min(Time, Duration);
	}
	else
	{
		Time = FMath::Min(Time + FMath::Min(Seconds, MaxStep), Duration);
	}
	bFlying = Time < Duration;
}

void FUghIntro::Hurry()
{
	if (!bFlying || bHurried)
	{
		return;
	}
	if (Shown() == 0)
	{
		// nothing seen yet: the flight starts nearer instead of rushing the whole of it
		Time = FMath::Max(Time, Duration - SkipTo);
	}
	if (Duration - Time <= HurrySeconds)
	{
		return;   // it ends in time as it is
	}
	bHurried = true;
	Hurried = 0;
	HurryFrom = Time;
	// the rest of the way (S, the share of it, 0 .. 1) in HurrySeconds as a quintic of the share of that time: from the
	// share, speed and acceleration it has when the key comes to a stop with no deceleration left - the smoothest way
	// (the least jerk) there, no jump of the speed
	double Rate, Change;
	ShareRates(Time / Duration, Rate, Change);
	const double P0 = Share(Time / Duration), P1 = Rate * HurrySeconds / Duration,
		P2 = Change * FMath::Square(HurrySeconds / Duration) / 2;
	const double A = 1 - P0 - P1 - P2, B = -(P1 + 2 * P2), C = -2 * P2;
	Hurry0 = P0;
	Hurry1 = P1;
	Hurry2 = P2;
	Hurry3 = 10 * A - 4 * B + C / 2;
	Hurry4 = -15 * A + 7 * B - C;
	Hurry5 = 6 * A - 3 * B + C / 2;
	// only when it goes on forward all the way (else, near the end, its slow end would overshoot): else a clock that
	// speeds up smoothly from its pace, Time = From + r + Hurry3 r^3
	bHurryAlong = true;
	for (int32 Sample = 1; Sample <= 32 && bHurryAlong; ++Sample)
	{
		const double R = Sample / 32.0;
		bHurryAlong = P1 + R * (2 * P2 + R * (3 * Hurry3 + R * (4 * Hurry4 + R * 5 * Hurry5))) >= -1e-9;
	}
	if (!bHurryAlong)
	{
		Hurry3 = (Duration - Time - HurrySeconds) / FMath::Cube(HurrySeconds);
	}
}

FUghCameraPose FUghIntro::At(const FUghCameraPose& End, double SeaZ, double Time)
{
	if (Time >= Duration)
	{
		return End;
	}
	const double U = FMath::Max(Time, 0.0) / Duration;
	const FWay Way(End, SeaZ);
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
