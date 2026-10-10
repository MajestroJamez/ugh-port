#include "UghVoyage.h"

namespace
{
	/** From the play: pulled back from the game's camera this far (units), up this much. */
	constexpr double PullBack = 9000, PullUp = 3000;
	/** From the level selection: beside the chosen stone this far (units), this high above the sea. */
	constexpr double Beside = 26000, BesideAbove = 11000;
	/**
	 * The mist: this far in front of the middle between the two stones' cameras (from the selection: in front of the
	 * next stone's, to the side), this high above the sea (above every stone of the archipelago, FUghIsles::Heights).
	 */
	constexpr double MistOut = 22000, MistAside = 6000, MistAbove = 13000;
	/** Then down to the stone: in front of its camera this far, to the right, this high above the sea (as FUghIntro). */
	constexpr double NearOut = 12000, NearRight = 2000, NearAbove = 4000;
	constexpr double LastOut = 4500, LastRight = -500, LastAbove = 1800;
	/** On the way: the lens (degrees across), the exposure out in the daylight (EV), its motion blur (FUghIntro's). */
	constexpr float Lens = 55, Outdoors = -0.8f, FlightBlur = 0.2f;
	/** It banks like FUghIntro: at most, how hard a turn banks it most of that, over how long; positions apart. */
	constexpr double MaxRoll = 10, RollAccel = 5000, TurnSeconds = 0.6, Step = 0.06;
	/** The mist thins this fast when hurried (seconds). */
	constexpr double MistOutHurried = 0.35;

	using UghFlight::FWay;

	FVector Place(const FWay& Way, double Duration, double Time)
	{
		return Way.Along(UghFlight::Share(FMath::Clamp(Time / Duration, 0.0, 1.0), FUghVoyage::Profile));
	}

	/** Where the camera `Pose` looks at the plane of the play (world y 0, UghShapes): the level. */
	FVector LevelSeen(const FUghCameraPose& Pose)
	{
		const FVector View = Pose.Rotation.Vector();
		const double Reach = FMath::Abs(View.Y) > 0.1 ? -Pose.Location.Y / View.Y : 7000;
		return Pose.Location + View * Reach;
	}
}

TArray<FVector> FUghVoyage::Waypoints(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift,
	const FVector& Stone, bool bFromPlay, double SeaZ, int32& MistPoint)
{
	const FRotator Heading(0, Game.Rotation.Yaw, 0);
	const FVector Forward = Heading.Vector(), Right = FRotationMatrix(Heading).GetUnitAxis(EAxis::Y);
	const FVector Start = Game.Location, End = Game.Location + Shift;
	TArray<FVector> Points{ From.Location };
	FVector Mist;
	if (bFromPlay)
	{
		Points.Add(Start - Forward * PullBack + FVector(0, 0, PullUp));
		Mist = (Start + End) / 2 - Forward * MistOut;
	}
	else
	{
		// around the chosen stone on the side the camera is on, to its front
		const FVector Chosen = Stone + Shift;
		const double Side = ((From.Location - Chosen) | Right) >= 0 ? 1 : -1;
		FVector Around = Chosen + Right * Side * Beside;
		Around.Z = SeaZ + BesideAbove;
		Points.Add(Around);
		Mist = End - Forward * MistOut + Right * Side * MistAside;
	}
	Mist.Z = SeaZ + MistAbove;
	MistPoint = Points.Add(Mist);
	FVector Near = End - Forward * NearOut + Right * NearRight;
	Near.Z = SeaZ + NearAbove;
	FVector Last = End - Forward * LastOut + Right * LastRight;
	Last.Z = FMath::Lerp(SeaZ + LastAbove, End.Z, 0.5);
	Points.Append({ Near, Last, End });
	return Points;
}

double FUghVoyage::MistTime(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift,
	const FVector& Stone, bool bFromPlay, double SeaZ, double Duration)
{
	int32 MistPoint = 0;
	const FWay Way(Waypoints(From, Game, Shift, Stone, bFromPlay, SeaZ, MistPoint));
	// the length of the curve up to the mist's point
	constexpr int32 Samples = 256;
	double Length = 0;
	FVector Previous = Way.At(0);
	for (int32 Sample = 1; Sample <= Samples; ++Sample)
	{
		const FVector Next = Way.At(Way.Knots[MistPoint] * Sample / Samples);
		Length += FVector::Dist(Previous, Next);
		Previous = Next;
	}
	const double U = UghFlight::ShareAt(FMath::Clamp(Length / Way.Length(), 0.0, 1.0), 0, Profile);
	return FMath::Clamp(U * Duration, MistIn + 0.5, Duration - 2);
}

void FUghVoyage::Start(const FUghCameraPose& InFrom, const FUghCameraPose& Game, const FVector& InShift,
	const FVector& InStone, bool bInFromPlay, double SeaZ)
{
	From = InFrom;
	Shift = InShift;
	Stone = InStone;
	bFromPlay = bInFromPlay;
	const double Duration = bFromPlay ? FromPlay : FromIsles;
	Clock.Start(Duration);
	SwitchTime = MistTime(From, Game, Shift, Stone, bFromPlay, SeaZ, Duration);
	bFlying = true;
	bSwitched = bHurried = bArrived = false;
	Mist = 0;
}

void FUghVoyage::Advance(double Seconds)
{
	if (!bFlying)
	{
		return;
	}
	if (!bSwitched)
	{
		// on to the mist's middle, held there until the world has switched
		const double Room = FMath::Max(SwitchTime - Clock.GetTime(), 0.0);
		Clock.Advance(FMath::Min(FMath::Min(Seconds, MaxStep), Room));
		const double Thick = FMath::Clamp((Clock.GetTime() - (SwitchTime - MistIn)) / MistIn, 0.0, 1.0);
		Mist = FMath::Max(Thick, bHurried ? FMath::Min(Mist + Seconds / MistHurried, 1.0) : Mist);
		return;
	}
	Clock.Advance(Seconds, MaxStep);
	Mist = FMath::Max(Mist - Seconds / (Clock.IsHurried() ? MistOutHurried : MistOut), 0.0);
	if (!Clock.IsRunning())
	{
		bFlying = false;
		bArrived = true;
	}
}

void FUghVoyage::Hurry()
{
	if (!bFlying)
	{
		return;
	}
	if (!bSwitched)
	{
		bHurried = true;   // into the mist at once; on from SkipTo before its end after it (Switched)
	}
	else
	{
		Clock.Hurry();
	}
}

void FUghVoyage::Switched()
{
	bSwitched = true;
	if (bHurried)
	{
		Clock.SkipTo(Clock.GetDuration() - SkipTo);   // (in the mist: not seen)
		Clock.Hurry();
	}
}

FUghCameraPose FUghVoyage::Along(const FUghCameraPose& Game, double SeaZ) const
{
	return At(From, Game, Shift, Stone, bFromPlay, SeaZ, Clock.GetDuration(), Clock.GetTime());
}

FUghCameraPose FUghVoyage::Pose(const FUghCameraPose& Game, double SeaZ) const
{
	FUghCameraPose Now = Along(Game, SeaZ);
	if (bSwitched)
	{
		Now.Location -= Shift;
	}
	return Now;
}

FUghCameraPose FUghVoyage::At(const FUghCameraPose& From, const FUghCameraPose& Game, const FVector& Shift,
	const FVector& Stone, bool bFromPlay, double SeaZ, double Duration, double Time)
{
	FUghCameraPose End = Game;
	End.Location += Shift;
	if (Time >= Duration)
	{
		return End;
	}
	const double U = FMath::Clamp(Time / Duration, 0.0, 1.0);
	int32 MistPoint = 0;
	const FWay Way(Waypoints(From, Game, Shift, Stone, bFromPlay, SeaZ, MistPoint));
	FUghCameraPose Pose;
	Pose.Location = Place(Way, Duration, Time);
	// it looks at the level it leaves (from the selection: the chosen stone), then at the level it goes to
	const FVector Left = bFromPlay ? LevelSeen(Game) : Stone + Shift + FVector(0, 0, 3000);
	const FVector Next = LevelSeen(Game) + Shift;   // (its level's plane is Shift away too)
	const FVector Look = FMath::Lerp(Left, Next,
		FMath::SmoothStep(bFromPlay ? 0.2 : 0.0, bFromPlay ? 0.6 : 0.5, U)) - Pose.Location;
	// from the way it looked at its start (never upside down: the direction blended, its roll its own)
	const FVector Facing = FMath::Lerp(From.Rotation.Vector(), Look.GetSafeNormal(), FMath::SmoothStep(0.0, 0.12, U));
	Pose.Rotation = Facing.GetSafeNormal().Rotation();
	// banking into its turns (sideways to its way, the turn over TurnSeconds), level at its start and end
	const FVector Before = Place(Way, Duration, Time - TurnSeconds / 2), Now = Pose.Location,
		After = Place(Way, Duration, Time + TurnSeconds / 2);
	const FVector Heading = After - Before, Turn = (Before + After - 2 * Now) / FMath::Square(TurnSeconds / 2);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Heading.GetSafeNormal2D());
	Pose.Rotation.Roll = MaxRoll * FMath::Tanh((Turn | Side) / RollAccel) * FMath::SmoothStep(0.0, 0.15, U) *
		(1 - FMath::SmoothStep(0.6, 0.9, U));
	// its lens wider on the way, the exposure darker out in the daylight, a little blur; the game's camera's at its end
	const float Out = float(FMath::SmoothStep(0.0, 0.3, U)), In = float(FMath::SmoothStep(0.55, 1.0, U));
	Pose.FieldOfView = FMath::Lerp(FMath::Lerp(From.FieldOfView, Lens, Out), End.FieldOfView, In);
	Pose.ExposureBias = FMath::Lerp(FMath::Lerp(From.ExposureBias, Outdoors, Out), End.ExposureBias, In);
	Pose.MotionBlur = FMath::Lerp(FMath::Lerp(FMath::Max(From.MotionBlur, 0.f), FlightBlur,
		float(FMath::SmoothStep(0.0, 0.15, U))), FMath::Max(End.MotionBlur, 0.f), float(FMath::SmoothStep(0.85, 1.0, U)));
	return Pose;
}
