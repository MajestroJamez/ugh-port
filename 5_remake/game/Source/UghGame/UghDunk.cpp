#include "UghDunk.h"

#include "UghBetween.h"
#include "UghShapes.h"
#include "UghSimulation.h"
#include "UghWater.h"

namespace
{
	/** The middle of the body across, in pixels from the copter's corner. */
	constexpr double BodyMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;

	int32 PixelsOf(int32 Subpixels)
	{
		return FMath::FloorToInt32(double(Subpixels) / UghShapes::Subpixels);
	}

	/** A swing of `Amplitude` dying away: 0 at first, up first. */
	double Swing(double Amplitude, double Time, double Period, double Seconds)
	{
		return Amplitude * FMath::Sin(UE_TWO_PI * Time / Period) * FMath::Exp(-Time / Seconds);
	}
}

int32 FUghDunks::DepthOf(int32 Y, int32 WaterLevel)
{
	return PixelsOf(Y) - PixelsOf(WaterLevel) + Waterline;
}

void FUghDunks::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds)
{
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || &From == &Current)
	{
		Reset();   // between attempts and levels nothing goes on
		return;
	}
	Since = Since >= 0 ? Since + Seconds : Since;
	const double Surface = UghWater::Surface(Previous, Current, Alpha);
	for (int32 Player = 0; Player < UE_ARRAY_COUNT(Copters); ++Player)
	{
		FCopter& C = Copters[Player];
		if (Player >= Current.copter_count)
		{
			C = FCopter();
			continue;
		}
		const ugh_logic_copter& Now = Current.copters[Player];
		const ugh_logic_copter& Before = Player < From.copter_count ? From.copters[Player] : Now;
		const FVector2D Seen = UghBetween::Position(Before.x, Before.y, Now.x, Now.y, Alpha);
		const double SeenWaterline = Seen.Y + Waterline;
		const double Speed = double(Now.y - Before.y) / UghShapes::Subpixels;   // pixels a step, down
		const int32 Depth = DepthOf(Now.y, Current.water_level);
		C.Fell = C.Fell >= 0 ? C.Fell + Seconds : C.Fell;
		C.BobTime += Seconds;
		C.RollTime += Seconds;
		if (C.bSeen && C.Waterline < C.Surface && SeenWaterline >= Surface && Speed >= MinSpeed)
		{
			// its waterline reaches the surface: a splash, the water churning, a rock
			const double Share = FMath::Min(Speed / FullSpeed, 1.0);
			FUghDunkSplash Splash;
			Splash.Player = Player;
			Splash.Place = FVector2D(Seen.X + BodyMiddle, Surface);
			Splash.Scale = FMath::Clamp(Speed / FullSpeed, MinScale, MaxScale);
			Splashes.Add(Splash);
			C.Fell = 0;
			C.FellScale = Splash.Scale;
			C.FellAt = Splash.Place;
			C.RollTime = 0;
			C.Roll = MaxRoll * Share * (Now.x >= Before.x ? 1 : -1);
			C.Bob = 0;
			C.Hold = 1;
			Since = 0;
		}
		// under the water rising: how fast, for when it stops at the surface
		if (C.bSeen && Now.y != C.LastY)
		{
			C.Rise = Depth > 0 && Now.y < C.LastY ? double(C.LastY - Now.y) / UghShapes::Subpixels
				: Depth > 0 ? 0 : C.Rise;
		}
		if (C.bSeen && C.LastDepth > 0 && Depth == 0)
		{
			// up at the surface, stopped dead: it bobs on with the speed it came up at, foaming
			const double Omega = UE_TWO_PI / BobPeriod;
			C.BobTime = 0;
			C.Bob = FMath::Min(C.Rise * FUghSimulation::TickRate / Omega, MaxBob);
			C.Hold = 1;
			if (C.Rise >= MinFoamRise)
			{
				FUghDunkSplash Foam;
				Foam.Player = Player;
				Foam.Place = FVector2D(Seen.X + BodyMiddle, Surface);
				Foam.Scale = FoamScale;
				Foam.bSurfacing = true;
				Splashes.Add(Foam);
			}
			C.Rise = 0;
		}
		if (Depth < 0)
		{
			C.Hold = FMath::Max(C.Hold - Seconds / EndSeconds, 0.0);   // taken off: no more bobbing
		}
		C.bSeen = true;
		C.Waterline = SeenWaterline;
		C.Surface = Surface;
		C.LastY = Now.y;
		C.LastDepth = Depth;
	}
}

TArray<FUghDunkSplash> FUghDunks::TakeSplashes()
{
	return MoveTemp(Splashes);
}

FUghCopterBob FUghDunks::Of(int32 Player) const
{
	FUghCopterBob Bob;
	if (Player < 0 || Player >= UE_ARRAY_COUNT(Copters))
	{
		return Bob;
	}
	const FCopter& C = Copters[Player];
	Bob.Lift = C.Hold * Swing(C.Bob, C.BobTime, BobPeriod, BobSeconds);
	Bob.Roll = C.Hold * Swing(C.Roll, C.RollTime, RollPeriod, RollSeconds);
	return Bob;
}

TArray<FVector4> FUghDunks::Stirs(double Surface) const
{
	TArray<FVector4> Stirs;
	for (const FCopter& C : Copters)
	{
		if (C.Fell >= 0 && C.Fell < StirSeconds)
		{
			const double Left = 1 - C.Fell / StirSeconds;
			Stirs.Add(FVector4(UghShapes::ToWorld(C.FellAt.X, Surface, 0), StirStrength * C.FellScale * Left * Left));
		}
	}
	return Stirs;
}

TOptional<double> FUghDunks::Age() const
{
	return Since >= 0 ? Since : TOptional<double>();
}

void FUghDunks::Reset()
{
	for (FCopter& C : Copters)
	{
		C = FCopter();
	}
	Splashes.Reset();
	Since = -1;
}
