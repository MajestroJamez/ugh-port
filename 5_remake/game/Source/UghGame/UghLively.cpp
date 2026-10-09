#include "UghLively.h"

#include "UghBetween.h"
#include "UghFigureActions.h"
#include "UghShapes.h"

namespace
{
	bool Is(const TCHAR* Action, const TCHAR* Name) { return FCString::Strcmp(Action, Name) == 0; }

	/** The middle of a copter's body whose corner is `Corner` (pixels). */
	FVector2D BodyMiddle(const FVector2D& Corner)
	{
		return Corner + FVector2D((UghShapes::CopterBodyLeft + UghShapes::CopterBodyRight) / 2.0,
			UghShapes::CopterBodyHeight / 2.0);
	}
}

bool FUghLively::IsClose(const FVector2D& Corner, double Speed, const FVector2D& At, const FIntPoint& Size,
	bool bAlready)
{
	const double Left = Corner.X + UghShapes::CopterBodyLeft, Right = Corner.X + UghShapes::CopterBodyRight;
	const double Bottom = Corner.Y + UghShapes::CopterBodyHeight;
	const double Gap = FMath::Max3(0.0, Left - (At.X + Size.X), At.X - Right);
	return Gap <= (bAlready ? Apart : Close) && Bottom >= At.Y - (bAlready ? AboveOut : Above) &&
		Bottom <= At.Y + Size.Y - Clear && Speed >= Moving;
}

bool FUghLively::IsNear(const FVector2D& Corner, const FVector2D& At, const FIntPoint& Size, bool bAlready)
{
	return FVector2D::Distance(BodyMiddle(Corner), At + FVector2D(Size) / 2) <= (bAlready ? Far : Near);
}

void FUghLively::Begin()
{
	for (TPair<int32, FState>& State : States)
	{
		State.Value.bSeen = false;
	}
}

EUghLively FUghLively::Apply(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Alpha,
	double Seconds, const ugh_logic_entity& Passenger, const ugh_logic_entity* Before, const FVector2D& At,
	const FIntPoint& Size, FUghFigureAction& Action)
{
	FState& State = States.FindOrAdd(Passenger.index);
	State.bSeen = true;
	State.Middle = At + FVector2D(Size) / 2;
	const bool bPerson = Action.Model == EUghModel::Caveman && !Action.bInWater && Action.Door == 0;
	const bool bStanding = bPerson && (Is(Action.Action, TEXT("idle")) || Is(Action.Action, TEXT("wave")));
	const bool bWalking = bPerson && Is(Action.Action, TEXT("walk"));
	const double Lasted = State.Seconds + Seconds;   // (what it shows, until this frame)
	EUghLively Next = EUghLively::None;
	if (bStanding)
	{
		bool bClose = false, bNear = Is(Action.Action, TEXT("wave"));   // (the logic has it wave impatiently)
		for (int32 Copter = 0; Copter < Current.copter_count; ++Copter)
		{
			const ugh_logic_copter& From = Previous.copters[FMath::Min(Copter, Previous.copter_count - 1)];
			const ugh_logic_copter& To = Current.copters[Copter];
			const bool bMoved = Copter < Previous.copter_count && UghBetween::Moved(From.x, From.y, To.x, To.y);
			const FVector2D Corner = bMoved ? UghBetween::Position(From.x, From.y, To.x, To.y, Alpha)
				: UghBetween::Pixels(To.x, To.y);
			const double Speed = bMoved
				? (UghBetween::Pixels(To.x, To.y) - UghBetween::Pixels(From.x, From.y)).Size() : 0;
			bClose |= IsClose(Corner, Speed, At, Size, State.State == EUghLively::Duck);
			bNear |= IsNear(Corner, At, Size, State.State == EUghLively::Wave || State.State == EUghLively::Duck);
		}
		Next = bClose ? EUghLively::Duck : bNear ? EUghLively::Wave : EUghLively::Idle;
		// a duck, a wave lasts a while
		if (State.State == EUghLively::Duck && Next != EUghLively::Duck && Lasted < DuckHold)
		{
			Next = EUghLively::Duck;
		}
		else if (State.State == EUghLively::Wave && Next == EUghLively::Idle && Lasted < WaveHold)
		{
			Next = EUghLively::Wave;
		}
	}
	else if (bWalking)
	{
		// shown walking off the copter it rode in hidden: delivered
		const bool bDelivered = Before && Before->sprite < 0;
		if ((bDelivered && State.State != EUghLively::Joy) ||
			(State.State == EUghLively::Joy && Lasted < JoySeconds))
		{
			Next = EUghLively::Joy;
		}
	}
	if (Next == State.State)
	{
		State.Seconds = Lasted;
	}
	else
	{
		State.State = Next;
		State.Seconds = 0;
	}
	switch (Next)
	{
	case EUghLively::Wave: Action.Action = TEXT("wave"); Action.bFollowsFrames = false; break;
	case EUghLively::Duck: Action.Action = TEXT("duck"); Action.bFollowsFrames = false; break;
	case EUghLively::Joy: Action.Action = TEXT("cheer"); break;   // (its legs the walk's, on the sprite's frames)
	default: break;
	}
	return Next;
}

void FUghLively::End()
{
	for (auto It = States.CreateIterator(); It; ++It)
	{
		if (!It.Value().bSeen)
		{
			It.RemoveCurrent();
		}
	}
}

TOptional<FUghLively::FSeen> FUghLively::Longest(EUghLively State) const
{
	TOptional<FSeen> Found;
	for (const TPair<int32, FState>& Each : States)
	{
		if (Each.Value.State == State && (!Found || Each.Value.Seconds > Found->Seconds))
		{
			Found = static_cast<const FSeen&>(Each.Value);
		}
	}
	return Found;
}

EUghLively FUghLively::Of(int32 Index) const
{
	const FState* State = States.Find(Index);
	return State ? State->State : EUghLively::None;
}
