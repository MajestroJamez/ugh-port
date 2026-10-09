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

bool FUghSimulation::NewGame(const FUghGameChoice& Choice)
{
	if (!Logic)
	{
		return false;
	}
	ugh_logic_settings Settings;
	ugh_logic_default_settings(&Settings);
	Settings.players = Choice.Players;
	Settings.difficulty = Choice.Difficulty;
	Settings.first_level = Choice.FirstLevel;
	const int64 Ticks = FDateTime::Now().GetTicks();
	for (int32 Word = 0; Word < UE_ARRAY_COUNT(Settings.random_seed); ++Word)
	{
		Settings.random_seed[Word] = static_cast<uint16_t>(Ticks >> (16 * Word));
	}
	Result = ugh_logic_new_game(Logic, &Settings) ? UGH_LOGIC_CONTINUE : UGH_LOGIC_GAME_OVER;
	bWatching = false;
	RecordedLevel = -1;
	Ended.Reset();
	ugh_logic_get_view(Logic, &CurrentView);
	PreviousView = CurrentView;
	Waiting = 0;
	Events.Reset();
	return !IsOver();
}

void FUghSimulation::Preview(const FUghGameChoice& Choice)
{
	if (!NewGame(Choice))
	{
		return;
	}
	for (int32 Step = 0; Step < MaxPreviewSteps && CurrentView.level_id < 0; ++Step)
	{
		ugh_logic_step(Logic);
		ugh_logic_get_view(Logic, &CurrentView);
	}
	ugh_logic_take_events(Logic, [](void*, const ugh_logic_event*) {}, nullptr);   // a preview is silent
	PreviousView = CurrentView;
	Result = UGH_LOGIC_GAME_OVER;
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
	Events.Reset();
	if (!Logic || IsOver())
	{
		return;
	}
	Waiting += Seconds;
	int32 Steps = 0;
	while (Waiting * TickRate >= 1.0 && !IsOver())
	{
		Waiting -= 1.0 / TickRate;
		const int32 Stepped = ugh_logic_step(Logic);
		ugh_logic_view View;
		ugh_logic_get_view(Logic, &View);
		if (Stepped == UGH_LOGIC_REPLAY_OVER || (bWatching && View.level != WatchedLevel))
		{
			Result = UGH_LOGIC_REPLAY_OVER;   // (the next level's caption not shown)
			break;
		}
		Result = Stepped;
		PreviousView = CurrentView;
		CurrentView = View;
		AfterStep();
		if (++Steps == MaxStepsPerFrame)
		{
			Waiting = 0;
			break;
		}
	}
	ugh_logic_take_events(Logic, [](void* Context, const ugh_logic_event* Event)
		{ static_cast<TArray<ugh_logic_event>*>(Context)->Add(*Event); }, &Events);
	if (Result == UGH_LOGIC_REPLAY_OVER)
	{
		// (not the next level's caption and its jingle)
		Events.RemoveAll([](const ugh_logic_event& Event) { return Event.kind == UGH_LOGIC_EVENT_LEVEL_CAPTION; });
	}
}

void FUghSimulation::AfterStep()
{
	if (bWatching)
	{
		return;
	}
	// the logic records each level: the replay of one that ended (the game went on to the next, or ended)
	ugh_logic_attempt_start Start;
	if (ugh_logic_get_attempt_start(Logic, &Start) && Start.level != RecordedLevel)
	{
		if (RecordedLevel >= 0)
		{
			Ended = FUghReplay::Of(ugh_logic_get_replay(Logic, UGH_REPLAY_LEVEL_ENDED));
		}
		RecordedLevel = Start.level;
	}
	if (Result != UGH_LOGIC_CONTINUE && RecordedLevel >= 0)
	{
		Ended = FUghReplay::Of(ugh_logic_get_replay(Logic, UGH_REPLAY_LEVEL_ENDED));
		RecordedLevel = -1;
	}
}

bool FUghSimulation::Watch(const FUghReplay& Replay)
{
	if (!Logic || !ugh_logic_watch(Logic, Replay.GetHandle()))
	{
		return false;
	}
	bWatching = true;
	WatchedLevel = Replay.Level();
	RecordedLevel = -1;
	Ended.Reset();
	Result = UGH_LOGIC_CONTINUE;
	ugh_logic_get_view(Logic, &CurrentView);
	PreviousView = CurrentView;
	Waiting = 0;
	Events.Reset();
	return true;
}

int32 FUghSimulation::GetWatchedSteps() const
{
	return Logic ? ugh_logic_watched_steps(Logic) : -1;
}
