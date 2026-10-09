#include "UghPads.h"

#include "HAL/PlatformTime.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Xinput.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
	/** The connected pads are looked at again after this (seconds; XInput is slow to ask about a missing pad). */
	constexpr double LookAgain = 1;
}

void FUghPads::Set(int32 Pilot, const FUghRumble& Rumble)
{
	if (Pilot < 0 || Pilot >= FUghImpacts::Pilots || Sent[Pilot] == Rumble)
	{
		return;
	}
	Sent[Pilot] = Rumble;
#if PLATFORM_WINDOWS
	const int32 Pad = PadOf(Pilot);
	if (Pad != INDEX_NONE)
	{
		XINPUT_VIBRATION Vibration;
		Vibration.wLeftMotorSpeed = WORD(FMath::Clamp(Rumble.Large, 0.f, 1.f) * 65535.f);
		Vibration.wRightMotorSpeed = WORD(FMath::Clamp(Rumble.Small, 0.f, 1.f) * 65535.f);
		XInputSetState(DWORD(Pad), &Vibration);
	}
#endif
}

void FUghPads::StopAll()
{
	for (int32 Pilot = 0; Pilot < FUghImpacts::Pilots; ++Pilot)
	{
		if (!Sent[Pilot].IsOff())
		{
			Set(Pilot, FUghRumble());
		}
	}
}

int32 FUghPads::PadOf(int32 Pilot)
{
#if PLATFORM_WINDOWS
	const double Now = FPlatformTime::Seconds();
	if (Now - LookedAt >= LookAgain)
	{
		LookedAt = Now;
		Connected.Reset();
		for (DWORD Index = 0; Index < XUSER_MAX_COUNT; ++Index)
		{
			XINPUT_STATE State;
			if (XInputGetState(Index, &State) == ERROR_SUCCESS)
			{
				Connected.Add(int32(Index));
			}
		}
	}
	return Connected.IsValidIndex(Pilot) ? Connected[Pilot] : INDEX_NONE;
#else
	return INDEX_NONE;
#endif
}
