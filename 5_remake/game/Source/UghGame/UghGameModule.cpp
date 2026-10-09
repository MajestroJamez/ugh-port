#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <processthreadsapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
	/**
	 * The game's process out of Windows' power throttling: a process without a window in front (the autopilot's
	 * offscreen runs, the game behind another window) is run as "efficiency mode" (EcoQoS: slow clocks, the efficient
	 * cores) and its timers coarsened to 15.6 ms - frames of all threads several times longer in bursts, hitches of
	 * 50-200 ms the game never caused (step 29e). A game's frames want the speed whatever is in front.
	 */
	void OptOutOfPowerThrottling()
	{
#if PLATFORM_WINDOWS
		PROCESS_POWER_THROTTLING_STATE State{};
		State.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
		State.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED | PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
		State.StateMask = 0;   // (controlled and off: never throttled)
		const bool bDone = ::SetProcessInformation(::GetCurrentProcess(), ProcessPowerThrottling, &State, sizeof State) != 0;
		UE_LOG(LogTemp, Display, TEXT("UGH power throttling of the process: %s"), bDone ? TEXT("off") : TEXT("unchanged"));
#endif
	}

	class FUghGameModule : public FDefaultGameModuleImpl
	{
	public:
		virtual void StartupModule() override
		{
			OptOutOfPowerThrottling();
		}
	};
}

IMPLEMENT_PRIMARY_GAME_MODULE(FUghGameModule, UghGame, "UghGame");
