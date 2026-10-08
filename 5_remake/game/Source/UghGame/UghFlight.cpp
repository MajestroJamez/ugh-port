#include "UghFlight.h"

#include "Algo/BinarySearch.h"

namespace
{
	/** The curve sampled this many times (how far along it each is). */
	constexpr int32 Samples = 4096;

	/** Its speed at full speed: a share of the way in a share of the flight. */
	double FullSpeed(const UghFlight::FProfile& Profile)
	{
		return 1 / (Profile.Rise / 2 + (Profile.Cruise - Profile.Rise) + (1 - Profile.Cruise) / 2);
	}

	/** 6 w^5 - 15 w^4 + 10 w^3: 0 .. 1 with no change of speed or of its change at either end. */
	double Smootherstep(double W)
	{
		return W * W * W * (W * (6 * W - 15) + 10);
	}
}

double UghFlight::Share(double U, const FProfile& Profile)
{
	const double Full = FullSpeed(Profile), Rise = Profile.Rise, Cruise = Profile.Cruise;
	if (U < Rise && Rise > 0)
	{
		// the integral of smootherstep
		const double W = FMath::Max(U, 0.0) / Rise;
		return Full * Rise * (FMath::Pow(W, 6) - 3 * FMath::Pow(W, 5) + 2.5 * FMath::Pow(W, 4));
	}
	if (U < Cruise || Cruise >= 1)
	{
		return Full * (Rise / 2 + FMath::Min(U, 1.0) - Rise);
	}
	const double W = FMath::Min((U - Cruise) / (1 - Cruise), 1.0);
	// the integral of 1 - (6 w^5 - 15 w^4 + 10 w^3)
	const double Braked = W - 2.5 * FMath::Pow(W, 4) + 3 * FMath::Pow(W, 5) - FMath::Pow(W, 6);
	return Full * (Rise / 2 + Cruise - Rise + (1 - Cruise) * Braked);
}

void UghFlight::ShareRates(double U, const FProfile& Profile, double& Rate, double& Change)
{
	const double Full = FullSpeed(Profile), Rise = Profile.Rise, Cruise = Profile.Cruise;
	if (U < Rise && Rise > 0)
	{
		const double W = FMath::Max(U, 0.0) / Rise;
		Rate = Full * Smootherstep(W);
		Change = Full * 30 * FMath::Square(W) * FMath::Square(1 - W) / Rise;
		return;
	}
	if (U < Cruise || Cruise >= 1)
	{
		Rate = Full;
		Change = 0;
		return;
	}
	const double W = FMath::Min((U - Cruise) / (1 - Cruise), 1.0);
	Rate = Full * (1 - Smootherstep(W));
	Change = -Full * 30 * FMath::Square(W) * FMath::Square(1 - W) / (1 - Cruise);
}

double UghFlight::ShareAt(double S, double From, const FProfile& Profile)
{
	double Low = From, High = 1;
	for (int32 Halving = 0; Halving < 48; ++Halving)
	{
		const double Middle = (Low + High) / 2;
		(Share(Middle, Profile) < S ? Low : High) = Middle;
	}
	return (Low + High) / 2;
}

UghFlight::FWay::FWay(const TArray<FVector>& InPoints)
	: Points(InPoints)
{
	const int32 Count = Points.Num();
	check(Count >= 2);
	Knots.Add(0);
	for (int32 I = 1; I < Count; ++I)
	{
		Knots.Add(Knots.Last() + FMath::Max(FVector::Dist(Points[I - 1], Points[I]), 1e-3));
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
		const FVector Wanted = 6 * ((Points[I + 1] - Points[I]) / H1 - (Points[I] - Points[I - 1]) / H0);
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

FVector UghFlight::FWay::At(double T) const
{
	const int32 I = FMath::Clamp(Algo::UpperBound(Knots, T) - 1, 0, Knots.Num() - 2);
	const double H = Knots[I + 1] - Knots[I];
	const double A = (Knots[I + 1] - T) / H, B = (T - Knots[I]) / H;
	return A * Points[I] + B * Points[I + 1] + ((A * A * A - A) * Bends[I] + (B * B * B - B) * Bends[I + 1]) * (H * H / 6);
}

FVector UghFlight::FWay::Along(double S) const
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

void FUghFlightClock::Start(double NewDuration)
{
	if (NewDuration > 0)
	{
		Duration = NewDuration;
	}
	Time = 0;
	bRunning = true;
	bHurried = false;
	Hurried = HurryFrom = 0;
}

void FUghFlightClock::Advance(double Seconds, double MaxStep)
{
	if (!bRunning)
	{
		return;
	}
	if (bHurried)
	{
		// the rest of the way planned when the key came (Hurry), on the real clock
		Hurried = FMath::Min(Hurried + Seconds, HurrySeconds);
		const double R = Hurried / HurrySeconds;
		if (Hurried >= HurrySeconds)
		{
			Time = Duration;
		}
		else if (bHurryAlong)
		{
			const double S = Hurry0 + R * (Hurry1 + R * (Hurry2 + R * (Hurry3 + R * (Hurry4 + R * Hurry5))));
			Time = FMath::Max(Time, Duration * UghFlight::ShareAt(FMath::Min(S, 1.0), HurryFrom / Duration, Profile));
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
	bRunning = Time < Duration;
}

void FUghFlightClock::Hurry()
{
	if (!bRunning || bHurried || Duration - Time <= HurrySeconds)
	{
		return;   // (it ends in time as it is)
	}
	bHurried = true;
	Hurried = 0;
	HurryFrom = Time;
	// the rest of the way (S, the share of it, 0 .. 1) in HurrySeconds as a quintic of the share of that time: from the
	// share, speed and acceleration it has when the key comes to a stop with no deceleration left - the smoothest way
	// (the least jerk) there, no jump of the speed
	double Rate, Change;
	UghFlight::ShareRates(Time / Duration, Profile, Rate, Change);
	const double P0 = UghFlight::Share(Time / Duration, Profile), P1 = Rate * HurrySeconds / Duration,
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
