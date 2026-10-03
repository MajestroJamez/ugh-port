// The game logic at the original's tick.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"
#include "UghMenu.h"

/**
 * The game logic (its C API) stepped at the original's fixed tick, 70.086 Hz, whatever the frame rate: Advance runs
 * as many steps as the real time asks for and keeps the views before and after the last step, so a frame is drawn
 * between them (Alpha).
 */
class FUghSimulation
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
	/** The game is over (lost or won), not started or a preview: the logic is not stepped. */
	bool IsOver() const { return Result != UGH_LOGIC_CONTINUE; }
	/** UGH_LOGIC_GAME_OVER or UGH_LOGIC_ALL_LEVELS_DONE once over. */
	int32 GetResult() const { return Result; }

	/** A pilot's key (UGH_LOGIC_KEY_...), player 0 or 1. */
	void Key(int32 Player, int32 LogicKey, bool bPressed);
	/** A key the game loop sees (UGH_LOGIC_MENU_...). */
	void MenuKey(int32 LogicMenuKey);

	/** Runs the steps that `Seconds` more of real time ask for (at most MaxStepsPerFrame: a long hitch is dropped). */
	void Advance(double Seconds);

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
	double Waiting = 0;   // real time since the last step, seconds
	int32 Result = UGH_LOGIC_GAME_OVER;
};
