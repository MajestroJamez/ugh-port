#include "UghIsles.h"

#include "Math/RandomStream.h"
#include "UghControls.h"
#include "UghHighScores.h"

namespace
{
	/** A stone's place wanders this far (units) along its row and across it, its size this much either way. */
	constexpr double JitterAlong = 1000, JitterAcross = 800, ScaleSpread = 0.15;
	/** Where the camera choosing looks: this far beyond the stone (towards the next rows), at this share of its height. */
	constexpr double AimBeyond = 6000, AimHeight = 0.5;
	/** The camera over the archipelago: this far behind its middle and this high; its lens. */
	constexpr double OverBehind = 40000, OverHigh = 70000;
	constexpr float OverFieldOfView = 60;
	/**
	 * Looking down at the archipelago the exposure is darker by this much (EV): less than out over the sea in the title
	 * and the flights (-0.8; the sea seen from above mirrors little of the sky: dark)
	 */
	constexpr float FromAbove = -0.2f;
	/** Less blur than the engine's while it flies (its motion blur amount), as FUghIntro's. */
	constexpr float FlightBlur = 0.2f;
	/** Less still flying high over the archipelago (it passes fast). */
	constexpr float ArriveBlur = 0.1f;
	/** The flight to a stone passes this much (units) higher than straight between its ends, half way. */
	constexpr double ApproachArc = 4000;
	/** The glide is reckoned in steps no longer than this (seconds). */
	constexpr double GlideStep = 1.0 / 240;

	double Smootherstep(double W)
	{
		W = FMath::Clamp(W, 0.0, 1.0);
		return W * W * W * (W * (6 * W - 15) + 10);
	}

	/** A pose at `Eye` looking at `Aim`, level. */
	FUghCameraPose Looking(const FVector& Eye, const FVector& Aim, float FieldOfView, float Blur,
		float Exposure = FromAbove)
	{
		FUghCameraPose Pose;
		Pose.Location = Eye;
		Pose.Rotation = (Aim - Eye).Rotation();
		Pose.Rotation.Roll = 0;
		Pose.FieldOfView = FieldOfView;
		Pose.MotionBlur = Blur;
		Pose.ExposureBias = Exposure;
		return Pose;
	}
}

TArray<FUghIslePlace> FUghIsles::Layout(int32 Count, const FVector& Home)
{
	TArray<FUghIslePlace> Places;
	for (int32 Level = 0; Level < Count; ++Level)
	{
		FUghIslePlace Place;
		Place.Row = Level / PerRow;
		const int32 Along = Level % PerRow;
		// a winding path: left to right, then right to left in the row beyond it
		Place.Column = Place.Row % 2 == 0 ? Along : PerRow - 1 - Along;
		FRandomStream Random(7919 * Level + 104729);
		const double Across = (Place.Column - (PerRow - 1) / 2.0 + (Place.Row % 2 ? 0.25 : -0.25)) * ColumnGap;
		Place.Foot = FVector(Home.X + Across + Random.FRandRange(-1, 1) * JitterAlong,
			Home.Y - Nearest - Place.Row * RowGap + Random.FRandRange(-1, 1) * JitterAcross, 0);
		Place.Scale = 1 + Random.FRandRange(-1, 1) * ScaleSpread;
		Place.Yaw = Random.FRandRange(0, 360);
		Place.Variant = Random.RandHelper(Variants);
		Places.Add(Place);
	}
	return Places;
}

FVector FUghIsles::Top(const FUghIslePlace& Place)
{
	return Place.Foot + FVector(0, 0, Heights[Place.Variant] * Place.Scale);
}

int32 FUghIsles::Current(const FUghHighScores& Scores, int32 Players, int32 Count)
{
	const TArray<int32>& Done = Scores.Done(Players);
	const int32 Furthest = FMath::Max3(Scores.LastLevel(Players), Done.IsEmpty() ? 0 : Done.Last() + 1, 0);
	return FMath::Clamp(Furthest, 0, FMath::Max(Count - 1, 0));
}

EUghIsle FUghIsles::StateOf(const FUghHighScores& Scores, int32 Players, int32 Level, int32 Count, int32 Password)
{
	if (Scores.IsDone(Players, Level))
	{
		return EUghIsle::Done;
	}
	// the one got to, the first (no password needed, as in the original), a typed password's
	return Level == Current(Scores, Players, Count) || Level == 0 || Level == Password ? EUghIsle::Open : EUghIsle::Locked;
}

void FUghIsles::Open(int32 InPlayers, int32 Count, const FUghHighScores& Scores, int32 Password,
	const FUghCameraPose& InFrom, const FVector& InHome, double SeaZ, bool bFlight)
{
	Players = InPlayers;
	Home = InHome;
	Places = Layout(Count, Home);
	States.Reset();
	for (int32 Level = 0; Level < Count; ++Level)
	{
		States.Add(StateOf(Scores, Players, Level, Count, Password));
	}
	Cursor = Password >= 0 && Password < Count ? Password : Current(Scores, Players, Count);
	From = Pose = InFrom;
	Stage = bFlight ? EStage::Arrive : EStage::Choose;
	ArriveClock.Start(ArriveSeconds);
	StageTime = 0;
	bNewStage = true;
	Notice.Reset();
	NoticeAge = 1e9;
	ReturnAge = 1e9;
	Glide = FVector::ZeroVector;
}

FUghIsles::EResult FUghIsles::HandleKey(const FKey& Key)
{
	const bool bBack = Key == EKeys::Escape || Key == FUghControls::BackKey();
	switch (Stage)
	{
	case EStage::Arrive:
		if (bBack)
		{
			break;
		}
		ArriveClock.Hurry();   // any key
		return EResult::None;
	case EStage::Choose:
		if (Key == EKeys::Enter)
		{
			if (!IsSelectable(Cursor))
			{
				Notice = FString::Printf(TEXT("Level %d is locked: its password in the menu opens it"), Cursor + 1);
				NoticeAge = 0;
				return EResult::None;
			}
			From = Pose;
			Stage = EStage::Approach;
			StageTime = 0;
			bNewStage = true;   // its length from the camera's way (Advance)
			Notice.Reset();
			return EResult::Fly;
		}
		if (!bBack)
		{
			Cursor = Neighbour(Cursor, Key);
			return EResult::None;
		}
		break;
	case EStage::Approach:
		ApproachClock.Hurry();   // any key
		return EResult::None;
	default:
		return EResult::None;
	}
	// back to the title through black
	Stage = EStage::Leave;
	StageTime = 0;
	return EResult::None;
}

int32 FUghIsles::Neighbour(int32 Level, const FKey& Arrow) const
{
	if (!Places.IsValidIndex(Level))
	{
		return Level;
	}
	const FUghIslePlace& At = Places[Level];
	auto Find = [this](int32 Row, int32 Column)
	{
		return Places.IndexOfByPredicate([Row, Column](const FUghIslePlace& Place)
		{
			return Place.Row == Row && Place.Column == Column;
		});
	};
	if (Arrow == EKeys::Left || Arrow == EKeys::Right)
	{
		const int32 Beside = Find(At.Row, At.Column + (Arrow == EKeys::Right ? 1 : -1));
		if (Beside != INDEX_NONE)
		{
			return Beside;
		}
		// at the row's end on round it: the level before or after it in the next row
		for (const int32 Next : { Level + 1, Level - 1 })
		{
			if (Places.IsValidIndex(Next) && Places[Next].Row != At.Row)
			{
				return Next;
			}
		}
		return Level;
	}
	if (Arrow == EKeys::Up || Arrow == EKeys::Down)
	{
		const int32 Row = At.Row + (Arrow == EKeys::Up ? 1 : -1);
		int32 Closest = Level;
		double Best = TNumericLimits<double>::Max();
		for (int32 Other = 0; Other < Places.Num(); ++Other)
		{
			const double Apart = FMath::Abs(Places[Other].Foot.X - At.Foot.X);
			if (Places[Other].Row == Row && Apart < Best)
			{
				Best = Apart;
				Closest = Other;
			}
		}
		return Closest;
	}
	return Level;
}

FUghCameraPose FUghIsles::ChooseView(int32 Level, double SeaZ) const
{
	const FUghIslePlace& Place = Places[FMath::Clamp(Level, 0, Places.Num() - 1)];
	const FVector Target = Place.Foot + FVector(0, -AimBeyond, Heights[Place.Variant] * Place.Scale * AimHeight);
	const double Pitch = FMath::DegreesToRadians(ChoosePitch);
	const FVector Camera = Target + FVector(0, FMath::Cos(Pitch), FMath::Sin(Pitch)) * ChooseDistance;
	return Looking(Camera, Target, ChooseFieldOfView, 0);
}

FUghCameraPose FUghIsles::Overview(double SeaZ) const
{
	FBox Box(ForceInit);
	for (const FUghIslePlace& Place : Places)
	{
		Box += Place.Foot;
	}
	// high behind its first row (over the title's stone), looking out over it
	const FVector Middle = Box.GetCenter();
	return Looking(FVector(Middle.X, Box.Max.Y + OverBehind, Middle.Z + OverHigh), Middle, OverFieldOfView, FlightBlur);
}

FUghCameraPose FUghIsles::ApproachEnd(int32 Level, const FUghCameraPose& Game, double SeaZ) const
{
	// where the level's flight starts, the same way from its stone but nearer (the archipelago's are smaller)
	FUghCameraPose End = FUghIntro::At(Game, SeaZ, 0);
	const FUghIslePlace& Place = Places[FMath::Clamp(Level, 0, Places.Num() - 1)];
	const FVector Away = (End.Location - Home).GetSafeNormal2D();
	const double Above = End.Location.Z - SeaZ;
	End.Location = Place.Foot + Away * ApproachReach;
	End.Location.Z = Place.Foot.Z + FMath::Max(SeaZ - Place.Foot.Z, 0.0) + Above;
	return End;
}

double FUghIsles::Shown() const
{
	switch (Stage)
	{
	case EStage::Closed: return FMath::Clamp(ReturnAge / FadeSeconds, 0.0, 1.0);
	case EStage::Leave: return 1 - FMath::Clamp(StageTime / FadeSeconds, 0.0, 1.0);
	case EStage::Arrived: return 0;
	case EStage::Approach:
		return bNewStage ? 1 : 1 - FMath::SmoothStep(ApproachClock.GetDuration() - FadeSeconds,
			ApproachClock.GetDuration(), ApproachClock.GetTime());
	default: return 1;
	}
}

double FUghIsles::GetStageTime() const
{
	return Stage == EStage::Arrive ? ArriveClock.GetTime() : Stage == EStage::Approach ? ApproachClock.GetTime()
		: StageTime;
}

void FUghIsles::Advance(double Seconds, const FUghCameraPose& Game, double SeaZ)
{
	NoticeAge += Seconds;
	if (Stage == EStage::Closed)
	{
		ReturnAge += Seconds;
		return;
	}
	StageTime += Seconds;
	switch (Stage)
	{
	case EStage::Arrive:
		ArriveClock.Advance(Seconds, FUghIntro::MaxStep);
		Pose = FlightPose(Game, SeaZ);
		if (!ArriveClock.IsRunning())
		{
			Stage = EStage::Choose;
			StageTime = 0;
			bNewStage = true;
		}
		break;
	case EStage::Choose:
	{
		const FUghCameraPose Wanted = ChooseView(Cursor, SeaZ);
		EyeGoal = Wanted.Location;
		AimGoal = Wanted.Location + Wanted.Rotation.Vector() * ChooseDistance;
		if (bNewStage)
		{
			// (from the flight's end: the same view)
			Eye = EyeGoal;
			Aim = AimGoal;
			EyeSpeed = AimSpeed = FVector::ZeroVector;
			bNewStage = false;
		}
		// a critically damped spring after the cursor's stone: no jump of the speed, no overshoot
		const double W = GlideRate;
		for (double Left = FMath::Min(Seconds, 0.25); Left > 0; Left -= GlideStep)
		{
			const double Dt = FMath::Min(Left, GlideStep);
			EyeSpeed += (W * W * (EyeGoal - Eye) - 2 * W * EyeSpeed) * Dt;
			AimSpeed += (W * W * (AimGoal - Aim) - 2 * W * AimSpeed) * Dt;
			Eye += EyeSpeed * Dt;
			Aim += AimSpeed * Dt;
		}
		Glide = EyeSpeed;
		Pose = Looking(Eye, Aim, ChooseFieldOfView, 0);
		break;
	}
	case EStage::Approach:
		if (bNewStage)
		{
			// as long as its way at the speed the level's flight starts with (from a stop, Rise: a share of full speed)
			const FUghCameraPose End = ApproachEnd(Cursor, Game, SeaZ);
			const UghFlight::FWay Way({ From.Location,
				FMath::Lerp(From.Location, End.Location, 0.5) + FVector(0, 0, ApproachArc), End.Location });
			const double Flown = (FUghIntro::At(Game, SeaZ, 0.05).Location - FUghIntro::At(Game, SeaZ, 0).Location).Size();
			const double Speed = FMath::Max(Flown / 0.05, 1000.0);
			const UghFlight::FProfile& Profile = ApproachClock.GetProfile();
			const double FullSpeed = 1 / (Profile.Rise / 2 + (Profile.Cruise - Profile.Rise));
			ApproachClock.Start(FMath::Clamp(FullSpeed * Way.Length() / Speed, ApproachMin, ApproachMax));
			bNewStage = false;
		}
		ApproachClock.Advance(Seconds, FUghIntro::MaxStep);
		Pose = FlightPose(Game, SeaZ);
		if (!ApproachClock.IsRunning())
		{
			Stage = EStage::Arrived;
			StageTime = 0;
		}
		break;
	case EStage::Leave:
		if (StageTime >= FadeSeconds)
		{
			Stage = EStage::Closed;
			ReturnAge = 0;
		}
		break;
	default:
		break;
	}
}

FUghCameraPose FUghIsles::FlightPose(const FUghCameraPose& Game, double SeaZ) const
{
	const bool bArrive = Stage == EStage::Arrive;
	const FUghCameraPose End = bArrive ? ChooseView(Cursor, SeaZ) : ApproachEnd(Cursor, Game, SeaZ);
	const double S = bArrive ? ArriveClock.GetShare() : ApproachClock.GetShare();
	// where it looks: from where the camera looked at first to where it looks at the end, as far as the stone's middle
	const FVector Ahead = bArrive ? Home : Places[Cursor].Foot;
	const FVector LookFrom =
		From.Location + From.Rotation.Vector() * FMath::Max(FVector::Dist(From.Location, Ahead), 1000.0);
	const FVector LookTo = End.Location + End.Rotation.Vector() * (bArrive ? ChooseDistance
		: FMath::Max(FVector::Dist(End.Location, Places[Cursor].Foot), 1000.0));
	FUghCameraPose Now;
	if (bArrive)
	{
		// back from the title's stone, up over the archipelago and down to the cursor's stone
		const FUghCameraPose Over = Overview(SeaZ);
		const FVector OverAim = Over.Location + Over.Rotation.Vector() * FVector::Dist(Over.Location, Home);
		const UghFlight::FWay Way({ From.Location, Over.Location, End.Location });
		const UghFlight::FWay Look({ LookFrom, OverAim, LookTo });
		Now = Looking(Way.Along(S), Look.Along(S), 0, FlightBlur);
		Now.FieldOfView = S < 0.5
			? FMath::Lerp(From.FieldOfView, OverFieldOfView, float(FMath::SmoothStep(0.0, 0.5, S)))
			: FMath::Lerp(OverFieldOfView, End.FieldOfView, float(FMath::SmoothStep(0.5, 1.0, S)));
		// its own blur from and into the still cameras'
		Now.MotionBlur = ArriveBlur * float(FMath::SmoothStep(0.0, 0.15, S) * (1 - FMath::SmoothStep(0.8, 1.0, S)));
		Now.ExposureBias = FMath::Lerp(From.ExposureBias, End.ExposureBias, float(FMath::SmoothStep(0.0, 1.0, S)));
		return Now;
	}
	const UghFlight::FWay Way({ From.Location, FMath::Lerp(From.Location, End.Location, 0.5) + FVector(0, 0, ApproachArc),
		End.Location });
	Now = Looking(Way.Along(S), FMath::Lerp(LookFrom, LookTo, Smootherstep(S)), 0, FlightBlur);
	Now.Rotation.Roll = End.Rotation.Roll * float(FMath::SmoothStep(0.5, 1.0, S));
	Now.FieldOfView = FMath::Lerp(From.FieldOfView, End.FieldOfView, float(FMath::SmoothStep(0.0, 1.0, S)));
	Now.MotionBlur = FlightBlur * float(FMath::SmoothStep(0.0, 0.15, S));
	Now.ExposureBias = FMath::Lerp(From.ExposureBias, End.ExposureBias, float(FMath::SmoothStep(0.0, 1.0, S)));
	return Now;
}

double FUghIsles::GetStageDuration() const
{
	return Stage == EStage::Arrive ? ArriveClock.GetDuration() : Stage == EStage::Approach ? ApproachClock.GetDuration()
		: 0;
}
