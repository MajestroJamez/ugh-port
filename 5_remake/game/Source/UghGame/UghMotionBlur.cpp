#include "UghMotionBlur.h"

#include "UghBetween.h"

double FUghMotionBlur::Speed(const ugh_logic_view& Previous, const ugh_logic_view& Current)
{
	if (Current.phase != UGH_LOGIC_PHASE_PLAY || Current.level_id < 0)
	{
		return 0;
	}
	const ugh_logic_view& From = UghBetween::From(Previous, Current);
	double Fastest = 0;
	for (int32 Player = 0; Player < Current.copter_count && Player < From.copter_count; ++Player)
	{
		const ugh_logic_copter& A = From.copters[Player];
		const ugh_logic_copter& B = Current.copters[Player];
		if (!UghBetween::Moved(A.x, A.y, B.x, B.y))
		{
			return -1;
		}
		Fastest = FMath::Max(Fastest, (UghBetween::Pixels(B.x, B.y) - UghBetween::Pixels(A.x, A.y)).Size());
	}
	return Fastest;
}

double FUghMotionBlur::AmountAt(double Speed)
{
	const double T = FMath::Clamp((Speed - SlowPixels) / (FastPixels - SlowPixels), 0.0, 1.0);
	return FMath::Lerp(Rest, Full, T * T * T * (T * (T * 6 - 15) + 10));
}

void FUghMotionBlur::Update(const ugh_logic_view& Previous, const ugh_logic_view& Current, double Seconds)
{
	const double Fastest = Speed(Previous, Current);
	bJumped = Fastest < 0;
	const double Wanted = AmountAt(FMath::Max(Fastest, 0.0));
	Amount = bJumped ? Rest : FMath::Lerp(Wanted, Amount, FMath::Exp(-FMath::Max(Seconds, 0.0) / FollowSeconds));
}
