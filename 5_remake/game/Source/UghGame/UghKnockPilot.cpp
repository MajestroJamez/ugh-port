#include "UghKnockPilot.h"

#include "UghShapes.h"

namespace
{
	/** It looks this many steps ahead at its speed (up and down), goes across at most this fast (pixels a step). */
	constexpr double Lead = 12, MaxSpeed = 1, SpeedPerPixel = 0.04, SpeedSlack = 0.05;
	/** A passenger's middle is about this far right of its sprite's corner (pixels). */
	constexpr double PassengerMiddle = 6;
	/** Its body's middle is this far right of its corner. */
	constexpr double BodyMiddle = (UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight + 1) / 2.0;

	/** A passenger on land out of its door (waiting, calling, walking): its sprite's name is "kind1.walkLeft" .. */
	bool IsOnLand(const ugh_logic* Logic, const ugh_logic_entity& Entity)
	{
		ugh_logic_sprite Info;
		if (Entity.kind != UGH_LOGIC_ENTITY_PASSENGER || Entity.sprite < 0 ||
			!ugh_logic_get_sprite(Logic, Entity.sprite, &Info))
		{
			return false;
		}
		const FString Name = UTF8_TO_TCHAR(Info.name);
		return Name.StartsWith(TEXT("kind")) && !Name.Contains(TEXT("-water")) && !Name.Contains(TEXT("comingOut")) &&
			!Name.Contains(TEXT("goingIn"));
	}

	/** The surface of the pad under a passenger at `X`, `Y` (pixels), none without one. */
	TOptional<int32> PadUnder(const ugh_logic* Logic, double X, double Y)
	{
		TOptional<int32> Surface;
		for (int32 Index = 0; Index < ugh_logic_pad_count(Logic); ++Index)
		{
			ugh_logic_pad Pad;
			if (ugh_logic_get_pad(Logic, Index, &Pad) && Pad.left - 8 <= X && X <= Pad.right + 8 && Pad.y > Y &&
				(!Surface || Pad.y < *Surface))
			{
				Surface = Pad.y;
			}
		}
		return Surface;
	}
}

FUghPilotKeys FUghKnockPilot::Fly(const ugh_logic* Logic, const ugh_logic_view& Previous,
	const ugh_logic_view& Current, bool bKnocked)
{
	FUghPilotKeys Keys;
	if (!Logic || Current.copter_count == 0 || Previous.copter_count == 0)
	{
		return Keys;
	}
	const ugh_logic_copter& Copter = Current.copters[0];
	const FVector2D At = FVector2D(Copter.x, Copter.y) / UghShapes::Subpixels;
	const FVector2D Speed = FVector2D(Copter.x - Previous.copters[0].x, Copter.y - Previous.copters[0].y) /
		UghShapes::Subpixels;
	FVector2D Target = At;   // hovering where it is
	if (!bKnocked)
	{
		for (int32 I = 0; I < Current.entity_count; ++I)
		{
			const ugh_logic_entity& Entity = Current.entities[I];
			const FVector2D Corner = FVector2D(Entity.x, Entity.y) / UghShapes::Subpixels;
			const TOptional<int32> Pad = IsOnLand(Logic, Entity)
				? PadUnder(Logic, Corner.X + PassengerMiddle, Corner.Y) : TOptional<int32>();
			if (Pad)
			{
				Target.Y = *Pad - Clearance - UghShapes::CopterBodyHeight;
				if (FMath::Abs(At.Y - Target.Y) < Level)
				{
					Target.X = Corner.X + PassengerMiddle - BodyMiddle;
				}
				break;
			}
		}
	}
	Keys.bUp = At.Y + Speed.Y * Lead > Target.Y;
	const double Wanted = FMath::Clamp((Target.X - At.X) * SpeedPerPixel, -MaxSpeed, MaxSpeed);
	Keys.bRight = Speed.X < Wanted - SpeedSlack;
	Keys.bLeft = Speed.X > Wanted + SpeedSlack;
	return Keys;
}
