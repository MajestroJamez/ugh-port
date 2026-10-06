#include "UghFling.h"

#include "UghBetween.h"
#include "UghShapes.h"
#include "UghSprites.h"
#include "UghWater.h"

namespace
{
	const ugh_logic_entity* PassengerIn(const ugh_logic_view& View, int32 Index)
	{
		for (int32 I = 0; I < View.entity_count; ++I)
		{
			const ugh_logic_entity& Entity = View.entities[I];
			if (Entity.kind == UGH_LOGIC_ENTITY_PASSENGER && Entity.index == Index && Entity.sprite >= 0)
			{
				return &Entity;
			}
		}
		return nullptr;
	}

	double WaterRow(const ugh_logic_view& View)
	{
		return double(View.water_level) / UghShapes::Subpixels;
	}
}

void FUghFlings::Load(const ugh_logic* Logic, const FUghSprites* InSprites)
{
	Sprites = InSprites;
	InWater.Init(false, Sprites ? Sprites->Count() : 0);
	for (int32 Sprite = 0; Sprite < InWater.Num(); ++Sprite)
	{
		ugh_logic_sprite Info;
		if (Logic && ugh_logic_get_sprite(Logic, Sprite, &Info))
		{
			FString Kind = UTF8_TO_TCHAR(Info.name), Animation;
			InWater[Sprite] = Kind.Split(TEXT("."), &Kind, &Animation) && Kind.EndsWith(TEXT("-water"));
		}
	}
	Reset();
}

void FUghFlings::OnEvent(const ugh_logic_event& Event, const ugh_logic_view& View)
{
	if (Event.kind != UGH_LOGIC_EVENT_PASSENGER_IN_WATER || View.phase != UGH_LOGIC_PHASE_PLAY)
	{
		return;
	}
	const ugh_logic_entity* Entity = PassengerIn(View, Event.entity);
	if (!Entity)
	{
		return;
	}
	// where it stood (it may have fallen a little in the steps of this frame after the one that knocked it off)
	const FVector2D Corner = UghBetween::Pixels(Entity->x, Entity->y);
	const double* Stood = LandTops.Find(Event.entity);
	FFling Fling;
	Fling.Start = Stood ? *Stood : Corner.Y;
	Fling.Middle = Corner.X + (Sprites ? Sprites->Size(Entity->sprite).X : 0) / 2.0;
	Fling.Drop = WaterRow(View) - EntryBelowTop - Fling.Start;
	if (Fling.Drop < MinDrop)
	{
		return;   // the water rose to it, or it stood at the water: it only goes in
	}
	const double Below = UghShapes::ScreenHeight + Room - WaterRow(View);
	Fling.Throw =
		FMath::Min(FMath::Clamp(Fling.Drop * ThrowPerPixel, MinThrow, MaxThrow), FMath::Max(Below, 0.0) * RoomThrow);
	Fling.Lift = FMath::Min(Fling.Drop * LiftShare, MaxLift);
	Flings.Add(Event.entity, Fling);
	Newest = Event.entity;
}

void FUghFlings::OnView(const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || &UghBetween::From(Previous, Current) == &Current)
	{
		Reset();   // between attempts and levels nothing goes on
		return;
	}
	LandTops.Reset();
	for (int32 I = 0; I < Current.entity_count; ++I)
	{
		const ugh_logic_entity& Entity = Current.entities[I];
		if (Entity.kind == UGH_LOGIC_ENTITY_PASSENGER && Entity.sprite >= 0 && !IsInWater(Entity.sprite))
		{
			LandTops.Add(Entity.index, UghBetween::Pixels(Entity.x, Entity.y).Y);
		}
	}
	for (auto It = Flings.CreateIterator(); It; ++It)
	{
		const ugh_logic_entity* Now = PassengerIn(Current, It.Key());
		const ugh_logic_entity* Before = PassengerIn(Previous, It.Key());
		FFling& Fling = It.Value();
		if (Now && Before && Now->y < Before->y)
		{
			Fling.bRising = true;
		}
		// over when the logic has it afloat (still after coming up), gone, or out of the water
		const bool bAfloat = Fling.bRising && Now && Before && Now->y == Before->y;
		const bool bOut = Now && !IsInWater(Now->sprite) && Fling.Splashed >= 0;
		if (!Now || bAfloat || bOut || Fling.Age > MaxSeconds)
		{
			It.RemoveCurrent();
		}
	}
}

void FUghFlings::Show(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha, double Seconds)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY)
	{
		return;
	}
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	const double Surface = UghWater::Surface(Previous, Current, Alpha);
	for (TPair<int32, FFling>& Each : Flings)
	{
		FFling& Fling = Each.Value;
		Fling.Age += Seconds;
		const ugh_logic_entity* Now = PassengerIn(Current, Each.Key);
		if (!Now || Fling.Splashed >= 0)
		{
			continue;
		}
		const ugh_logic_entity* Before = PassengerIn(From, Each.Key);
		const double Top = Before ? UghBetween::Position(Before->x, Before->y, Now->x, Now->y, Alpha).Y
			: UghBetween::Pixels(Now->x, Now->y).Y;
		if (Top + EntryBelowTop < Surface)
		{
			continue;
		}
		// its feet reach the water: it splashes in front of the stone, then goes back under the water
		Fling.Splashed = Fling.Age;
		Fling.Return = FMath::Clamp(ReturnShare * Fling.Age, MinReturn, MaxReturn);
		FUghFlingSplash Splash;
		Splash.Passenger = Each.Key;
		Splash.Place = FVector2D(Fling.Middle, Surface);
		Splash.Depth = -Fling.Throw;
		Splash.Scale = FMath::Min(1 + Fling.Drop * SplashPerPixel, MaxSplash);
		Splashes.Add(Splash);
	}
}

TArray<FUghFlingSplash> FUghFlings::TakeSplashes()
{
	return MoveTemp(Splashes);
}

TOptional<FUghFlight> FUghFlings::Of(int32 Passenger, double Top, double Surface) const
{
	const FFling* Fling = Flings.Find(Passenger);
	if (!Fling)
	{
		return {};
	}
	FUghFlight Flight;
	if (Fling->Splashed < 0)
	{
		// level towards the camera as the time goes (the square root of the share of the fall), a hump up at first
		const double Way = FMath::Max(Surface - EntryBelowTop - Fling->Start, 1.0);
		const double Time = FMath::Sqrt(FMath::Clamp((Top - Fling->Start) / Way, 0.0, 1.0));
		Flight.Depth = -Fling->Throw * Time;
		Flight.Lift = Fling->Lift * 4 * Time * (1 - Time);
		Flight.bInAir = true;
		return Flight;
	}
	const double Back = FMath::Clamp((Fling->Age - Fling->Splashed) / Fling->Return, 0.0, 1.0);
	const double Eased = Back * Back * Back * (Back * (Back * 6 - 15) + 10);   // smootherstep: no jolt
	Flight.Depth = -Fling->Throw * (1 - Eased);
	Flight.Lift = (1 - DiveShare) * FMath::Max(Top + HeadBelowTop - Surface, 0.0);   // not as deep
	return Flight;
}

const FUghFlings::FFling* FUghFlings::Last() const
{
	return Flings.Find(Newest);
}

TOptional<double> FUghFlings::Age() const
{
	const FFling* Fling = Last();
	return Fling ? Fling->Age : TOptional<double>();
}

TOptional<double> FUghFlings::SplashAge() const
{
	const FFling* Fling = Last();
	return Fling && Fling->Splashed >= 0 ? Fling->Splashed : TOptional<double>();
}

void FUghFlings::Reset()
{
	Flings.Reset();
	LandTops.Reset();
	Splashes.Reset();
	Newest = INDEX_NONE;
}
