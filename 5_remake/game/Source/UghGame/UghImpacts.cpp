#include "UghImpacts.h"

namespace
{
	/**
	 * How each impact is felt (EUghImpact's order): the crash hardest - a slow heavy swing, the motors at nearly full -,
	 * a stone on an enemy a short knock, a fall into the sea a soft heave, a bump into the soft edge a light tap.
	 */
	const FUghImpactFeel Feels[] = {
		{ 12, 13, 0.11, 0.9f, 0.45 },   // crash
		{ 5, 16, 0.07, 0.5f, 0.22 },    // a stone on an enemy
		{ 6, 9, 0.12, 0.55f, 0.35 },    // into the sea
		{ 3.5, 18, 0.05, 0.3f, 0.15 },  // a bump into the edge
	};
	static_assert(UE_ARRAY_COUNT(Feels) == int32(EUghImpact::Count));

	/** A swing dies away below this share of its start. */
	constexpr double Gone = 0.01;
	/** The small motor rumbles this much of the large one; the swing across its way this much of the swing along it. */
	constexpr float SmallMotor = 0.6f;
	constexpr double Across = 0.35, AcrossFrequency = 1.37;
	/** A bump into the edge as hard as its rustle (AUghFringe): its strength by its speed (pixels a second), within. */
	constexpr double BumpFullSpeed = 60, BumpLeast = 0.4, BumpMost = 1.3;
}

const FUghImpactFeel& FUghImpacts::FeelOf(EUghImpact Impact)
{
	return Feels[FMath::Clamp(int32(Impact), 0, int32(EUghImpact::Count) - 1)];
}

void FUghImpacts::SetShake(bool bOn)
{
	bShake = bOn;
	if (!bShake)
	{
		Shakes.Reset();
	}
}

void FUghImpacts::Reset()
{
	Shakes.Reset();
	for (TArray<FRumbling>& Each : Rumbles)
	{
		Each.Reset();
	}
	Dropper = INDEX_NONE;
}

void FUghImpacts::OnEvent(const ugh_logic_event& Event)
{
	switch (Event.kind)
	{
	case UGH_LOGIC_EVENT_COPTER_CRASHED:
		Add(EUghImpact::Crash, Event.player, 1);
		break;
	case UGH_LOGIC_EVENT_PASSENGER_DROPPED:
		Dropper = Event.player;   // its stone may hit an enemy
		break;
	case UGH_LOGIC_EVENT_ENEMY_STUNNED:   // (only a falling stone knocks an enemy out)
	case UGH_LOGIC_EVENT_TREE_DROP:       // (a stone bounced off the tree)
		Add(EUghImpact::StoneHit, Dropper, 1, FVector2D(0, 1));
		break;
	default:
		break;
	}
}

void FUghImpacts::OnDunk(int32 Player, double Scale)
{
	Add(EUghImpact::Dunk, Player, Scale, FVector2D(0, 1));
}

void FUghImpacts::OnBump(int32 Player, double Speed, bool bTop)
{
	Add(EUghImpact::Edge, Player, FMath::Clamp(Speed / BumpFullSpeed, BumpLeast, BumpMost),
		bTop ? FVector2D(0, 1) : FVector2D(1, 0));
}

void FUghImpacts::Add(EUghImpact Impact, int32 Player, double Strength, const FVector2D& Along)
{
	const FUghImpactFeel& Feel = FeelOf(Impact);
	++Counts[int32(Impact)];
	static const TCHAR* const Names[] = { TEXT("crash"), TEXT("stone hit"), TEXT("into the sea"), TEXT("edge bump") };
	UE_LOG(LogTemp, Display, TEXT("UGH impact: %s, pilot %d, strength %.2f%s"), Names[int32(Impact)], Player + 1,
		Strength, bShake ? TEXT("") : TEXT(" (no shake)"));
	if (bShake)
	{
		Shakes.Add({ Feel.Shake * Strength, Feel.Frequency, Feel.Damping, Along.GetSafeNormal() });
	}
	for (int32 Pilot = 0; Pilot < Pilots; ++Pilot)
	{
		if (Player == INDEX_NONE || Player == Pilot)
		{
			Rumbles[Pilot].Add({ FMath::Clamp(Feel.Rumble * float(Strength), 0.f, 1.f), Feel.Seconds });
		}
	}
}

void FUghImpacts::Advance(double Seconds)
{
	for (FShake& Shake : Shakes)
	{
		Shake.Age += Seconds;
	}
	Shakes.RemoveAll([](const FShake& Shake) { return FMath::Exp(-Shake.Age / Shake.Damping) < Gone; });
	for (TArray<FRumbling>& Each : Rumbles)
	{
		for (FRumbling& Rumbling : Each)
		{
			Rumbling.Age += Seconds;
		}
		Each.RemoveAll([](const FRumbling& Rumbling) { return Rumbling.Age >= Rumbling.Seconds; });
	}
}

FVector2D FUghImpacts::Offset() const
{
	FVector2D Sum = FVector2D::ZeroVector;
	for (const FShake& Shake : Shakes)
	{
		// a damped swing along its way (from the rest, the first swing the widest) and a smaller one across it
		const double Left = Shake.Amplitude * FMath::Exp(-Shake.Age / Shake.Damping);
		const double Phase = UE_TWO_PI * Shake.Frequency * Shake.Age;
		const FVector2D Perpendicular(-Shake.Along.Y, Shake.Along.X);
		Sum += Left * (Shake.Along * FMath::Sin(Phase) + Perpendicular * Across * FMath::Sin(Phase * AcrossFrequency));
	}
	return Sum.GetClampedToMaxSize(MostShake);
}

FUghRumble FUghImpacts::Rumble(int32 Player) const
{
	FUghRumble Rumble;
	if (Player < 0 || Player >= Pilots)
	{
		return Rumble;
	}
	for (const FRumbling& Rumbling : Rumbles[Player])
	{
		const float Left = 1.f - float(Rumbling.Age / Rumbling.Seconds);
		Rumble.Large = FMath::Max(Rumble.Large, Rumbling.Strength * Left * Left);
	}
	Rumble.Small = Rumble.Large * SmallMotor;
	return Rumble;
}
