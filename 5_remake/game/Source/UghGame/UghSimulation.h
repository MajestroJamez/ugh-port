// The game logic at the original's tick.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghControls.h"
#include "UghMenu.h"
#include "UghReplays.h"

/**
 * The game logic (its C API) stepped at the original's fixed tick, 70.086 Hz, whatever the frame rate: Advance runs
 * as many steps as the real time asks for and keeps the views before and after the last step, so a frame is drawn
 * between them (Alpha).
 */
class FUghSimulation : public IUghLogicInput
{
public:
	/** Steps of the logic a second: the frame rate of the original. */
	static constexpr double TickRate = int32(UGH_LOGIC_FRAMES_PER_1000_S) / 1000.0;

	FUghSimulation();
	~FUghSimulation();
	FUghSimulation(const FUghSimulation&) = delete;
	FUghSimulation& operator=(const FUghSimulation&) = delete;

	/** Loads the game data; false and the reason when it cannot. */
	bool Load(const FString& DataPath, FString& OutError);
	bool IsLoaded() const { return Logic != nullptr; }

	/** A new game as chosen, its random numbers seeded by the time; false when the logic refuses the choice. */
	bool NewGame(const FUghGameChoice& Choice);
	/**
	 * The first level of `Choice` without playing it (behind the menu): a new game stepped until the level is loaded;
	 * then it is over (IsOver) and Advance does nothing.
	 */
	void Preview(const FUghGameChoice& Choice);
	/** The game is over (lost or won), not started, a preview or a replay watched to its end: the logic is not stepped. */
	bool IsOver() const { return Result != UGH_LOGIC_CONTINUE; }
	/** UGH_LOGIC_GAME_OVER, UGH_LOGIC_ALL_LEVELS_DONE or UGH_LOGIC_REPLAY_OVER once over. */
	int32 GetResult() const { return Result; }

	/**
	 * Watches a replay of a level (ugh_logic_watch: the logic plays its keys, the players' are ignored) until it is over:
	 * all of its steps, or the step the logic goes on to the next level in (its caption is not shown: the view stays
	 * the level's). False when the logic refuses it (a level the data has not).
	 */
	bool Watch(const FUghReplay& Replay);
	bool IsWatching() const { return bWatching; }
	/** The steps of the replay watched so far. */
	int32 GetWatchedSteps() const;
	/**
	 * The replay of the level that ended in the steps of the last Advance (done, or the game ended in it; not while
	 * watching), taken: none when no level ended.
	 */
	TSharedPtr<FUghReplay> TakeEndedLevel() { return MoveTemp(Ended); }
	/** The steps of the play of the attempt being played so far (its first: 1); 0 out of the play. */
	int32 GetPlaySteps() const { return PlaySteps; }

	virtual void Key(int32 Player, int32 LogicKey, bool bPressed) override;
	virtual void MenuKey(int32 LogicMenuKey) override;

	/** Runs the steps that `Seconds` more of real time ask for (at most MaxStepsPerFrame: a long hitch is dropped). */
	void Advance(double Seconds);

	/** What happened in the steps of the last Advance (sounds, effects). */
	const TArray<ugh_logic_event>& GetEvents() const { return Events; }

	/** The view after the step before the last one, and after the last one. */
	const ugh_logic_view& GetPrevious() const { return PreviousView; }
	const ugh_logic_view& GetCurrent() const { return CurrentView; }
	/** Where real time is between the last step and the next one: 0 .. 1. */
	double Alpha() const { return FMath::Clamp(Waiting * TickRate, 0.0, 1.0); }

	/** The logic, for reading the background of the level being played. */
	const ugh_logic* GetLogic() const { return Logic; }

private:
	static constexpr int32 MaxStepsPerFrame = 8;
	/** A new game loads its first level within these steps (with its caption, after 9). */
	static constexpr int32 MaxPreviewSteps = 100;

	ugh_logic* Logic = nullptr;
	ugh_logic_view PreviousView;
	ugh_logic_view CurrentView;
	TArray<ugh_logic_event> Events;
	double Waiting = 0;   // real time since the last step, seconds
	int32 Result = UGH_LOGIC_GAME_OVER;
	bool bWatching = false;
	int32 WatchedLevel = -1;     // the level of the replay watched
	int32 RecordedLevel = -1;    // the level whose attempt the game plays (its replay is recorded), -1 none
	int32 PlaySteps = 0;
	TSharedPtr<FUghReplay> Ended;

	/** After a step: a level that ended (its replay to take), a replay watched over. */
	void AfterStep();
};
