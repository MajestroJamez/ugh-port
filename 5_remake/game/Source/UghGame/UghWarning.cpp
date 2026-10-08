#include "UghWarning.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarBlink(TEXT("ugh.Warning.Blink"), 1,
		TEXT("1: the warning over a fast copter blinks; 0: it shows steadily (a shot)"));

	/** The loudness of one axis: the impact against the crash limit, rock ahead. */
	int32 AxisLoudness(int32 Impact, bool bRock, int32 Limit)
	{
		if (!bRock || Limit <= 0 || Impact < Limit)
		{
			return 0;
		}
		const double Share = double(Impact - Limit) / FMath::Max(UGH_LOGIC_COPTER_TOP_SPEED - Limit, 1);
		return FMath::Clamp(1 + FMath::FloorToInt32(Share * UghWarning::Loudest), 1, UghWarning::Loudest);
	}
}

int32 UghWarning::Loudness(const ugh_logic_copter_danger& Danger)
{
	return FMath::Max(AxisLoudness(Danger.impact_x, Danger.rock_x != 0, Danger.crash_limit),
		AxisLoudness(Danger.impact_y, Danger.rock_y != 0, Danger.crash_limit));
}

int32 UghWarning::Of(const ugh_logic* Logic, int32 Player)
{
	ugh_logic_copter_danger Danger;
	return Logic && ugh_logic_get_copter_danger(Logic, Player, &Danger) ? Loudness(Danger) : 0;
}

bool UghWarning::IsLit(int32 Loudness, double Seconds)
{
	if (Loudness <= 0)
	{
		return false;
	}
	if (CVarBlink.GetValueOnGameThread() == 0)
	{
		return true;
	}
	const double Rate = BlinkRates[FMath::Clamp(Loudness, 1, Loudest) - 1];
	return FMath::Frac(Seconds * Rate) < BlinkShown;
}
