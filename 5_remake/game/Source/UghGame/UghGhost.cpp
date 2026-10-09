#include "UghGhost.h"

#include "HAL/PlatformTime.h"
#include "UghReplays.h"

FUghGhost::~FUghGhost()
{
	if (Logic)
	{
		ugh_logic_destroy(Logic);
	}
}

bool FUghGhost::Load(const FString& DataPath, FString& OutError)
{
	char Error[512] = "";
	Logic = ugh_logic_create(TCHAR_TO_UTF8(*DataPath), Error, sizeof Error);
	OutError = Logic ? FString() : FString::Printf(TEXT("%s: %s"), *DataPath, UTF8_TO_TCHAR(Error));
	return Logic != nullptr;
}

void FUghGhost::SetReplay(const TSharedPtr<FUghReplay>& InReplay)
{
	if (InReplay != Replay)
	{
		Replay = InReplay;
		Stage = EStage::None;
	}
}

void FUghGhost::Follow(int32 PlaySteps, double Budget)
{
	if (!Logic || !Replay)
	{
		Stage = EStage::None;
		return;
	}
	if (PlaySteps <= 0)
	{
		// between the plays: made ready for the next, a little a frame (its caption, the black before it)
		if (Stage != EStage::Aligning && Stage != EStage::Ready)
		{
			BeginAlign();
		}
		AlignSome(Budget);
		return;
	}
	if (Stage == EStage::Flying && PlaySteps < Steps)
	{
		Stage = EStage::None;   // (another play without a frame between: again)
	}
	if (Stage == EStage::None)
	{
		BeginAlign();
	}
	AlignSome(TNumericLimits<double>::Max());   // (ready already, unless it began in the play)
	if (Stage == EStage::Ready)
	{
		Stage = EStage::Flying;
	}
	while (Stage == EStage::Flying && Steps < PlaySteps)
	{
		if (!Step())
		{
			Stage = EStage::Over;
		}
	}
}

void FUghGhost::BeginAlign()
{
	Steps = 0;
	Attempts = 0;
	Stage = ugh_logic_watch(Logic, Replay->GetHandle()) ? EStage::Aligning : EStage::Over;
	UE_CLOG(Stage == EStage::Over, LogTemp, Warning, TEXT("UGH ghost: its logic refuses the replay"));
}

void FUghGhost::AlignSome(double Budget)
{
	const double Until = FPlatformTime::Seconds() + Budget;
	while (Stage == EStage::Aligning && FPlatformTime::Seconds() < Until)
	{
		// a few steps between looks at the clock
		for (int32 Each = 0; Each < AlignStepsAtOnce && Stage == EStage::Aligning; ++Each)
		{
			if (ugh_logic_step(Logic) != UGH_LOGIC_CONTINUE)
			{
				Stage = EStage::Over;   // (its last attempt's play never came)
				break;
			}
			ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
				{ *static_cast<int32*>(Context) += Event->kind == UGH_LOGIC_EVENT_LEVEL_CAPTION; }, &Attempts);
			ugh_logic_get_view(Logic, &Current);
			if (Attempts >= Replay->GetInfo().attempts && Current.phase == UGH_LOGIC_PHASE_PLAY)
			{
				Previous = Current;
				Steps = 1;   // (the play's first step, as the game's)
				Stage = EStage::Ready;
			}
		}
	}
}

bool FUghGhost::Step()
{
	const int32 Result = ugh_logic_step(Logic);
	ugh_logic_take_events(Logic, [](void*, const ugh_logic_event*) {}, nullptr);
	ugh_logic_view View;
	ugh_logic_get_view(Logic, &View);
	if (Result != UGH_LOGIC_CONTINUE || View.phase != UGH_LOGIC_PHASE_PLAY || View.level != Current.level)
	{
		return false;
	}
	Previous = Current;
	Current = View;
	++Steps;
	return true;
}

TSharedPtr<FUghReplay> FUghGhost::Of(const FUghReplays& Replays, int32 Players, int32 Level, bool bOn)
{
	const FUghReplays::FEntry* Best = bOn ? Replays.Best(Players, Level) : nullptr;
	return Best && Best->OtherLogic == 0 ? Best->Replay : nullptr;
}
