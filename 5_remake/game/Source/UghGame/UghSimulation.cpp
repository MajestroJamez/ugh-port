#include "UghSimulation.h"

#include "Misc/DateTime.h"

FUghSimulation::FUghSimulation()
	: PreviousView{}, CurrentView{}
{
	PreviousView.level_id = CurrentView.level_id = -1;
}

FUghSimulation::~FUghSimulation()
{
	if (Logic)
	{
		ugh_logic_destroy(Logic);
	}
}

bool FUghSimulation::Load(const FString& DataPath, FString& OutError)
{
	char Error[512] = "";
	Logic = ugh_logic_create(TCHAR_TO_UTF8(*DataPath), Error, sizeof Error);
	if (!Logic)
	{
		OutError = FString::Printf(TEXT("%s: %s"), *DataPath, UTF8_TO_TCHAR(Error));
	}
	return Logic != nullptr;
}

void FUghSimulation::NewGame()
{
	if (!Logic)
	{
		return;
	}
	ugh_logic_settings Settings;
	ugh_logic_default_settings(&Settings);
	const int64 Ticks = FDateTime::Now().GetTicks();
	for (int32 Word = 0; Word < UE_ARRAY_COUNT(Settings.random_seed); ++Word)
	{
		Settings.random_seed[Word] = static_cast<uint16_t>(Ticks >> (16 * Word));
	}
	Result = ugh_logic_new_game(Logic, &Settings) ? UGH_LOGIC_CONTINUE : UGH_LOGIC_GAME_OVER;
	ugh_logic_get_view(Logic, &CurrentView);
	PreviousView = CurrentView;
	Waiting = 0;
}

void FUghSimulation::Key(int32 Player, int32 LogicKey, bool bPressed)
{
	if (Logic)
	{
		ugh_logic_key(Logic, Player, LogicKey, bPressed ? 1 : 0);
	}
}

void FUghSimulation::MenuKey(int32 LogicMenuKey)
{
	if (Logic)
	{
		ugh_logic_menu_key(Logic, LogicMenuKey);
	}
}

void FUghSimulation::Advance(double Seconds)
{
	if (!Logic || IsOver())
	{
		return;
	}
	Waiting += Seconds;
	int32 Steps = 0;
	while (Waiting * TickRate >= 1.0 && !IsOver())
	{
		Waiting -= 1.0 / TickRate;
		Result = ugh_logic_step(Logic);
		PreviousView = CurrentView;
		ugh_logic_get_view(Logic, &CurrentView);
		if (++Steps == MaxStepsPerFrame)
		{
			Waiting = 0;
			break;
		}
	}
	ugh_logic_take_events(Logic, [](void*, const ugh_logic_event*) {}, nullptr);   // no sounds or effects yet
}
