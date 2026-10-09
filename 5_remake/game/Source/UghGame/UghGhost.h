// The ghost of the best run of a level.
#pragma once

#include "CoreMinimal.h"
#include "ugh_logic.h"

class FUghReplay;
class FUghReplays;

/**
 * The ghost of the best run of the level being played (Settings > Ghost): its replay (FUghReplays::Best) watched by a
 * logic of its own - a second instance, never touching the game's - in step with the game's play. Between the game's
 * plays (the caption, the black before the play) the ghost's logic is resumed at the replay's start and run on, a
 * little a frame (`Budget` seconds: no hitch), to the first step of the play of the replay's last attempt (the one that
 * did the level); when the game's play begins it makes a step for each of the game's. Shown (AUghGhosts) while both
 * play; gone when its run ends (its level done and faded out, the replay over) and back with the game's next attempt.
 * Only its copters are shown. Test `Ugh.Ghost` (`UghGhostTests.cpp`).
 */
class FUghGhost
{
public:
	/** Seconds a frame the ghost runs ahead to the start of its play, at most. */
	static constexpr double AlignBudget = 0.002;

	FUghGhost() : Previous{}, Current{} {}
	~FUghGhost();
	FUghGhost(const FUghGhost&) = delete;
	FUghGhost& operator=(const FUghGhost&) = delete;

	/** Its own logic of the game's data; false and why when it cannot be read. */
	bool Load(const FString& DataPath, FString& OutError);

	/** The replay the ghost flies (none: no ghost); a new one starts with the game's next play. */
	void SetReplay(const TSharedPtr<FUghReplay>& InReplay);
	const TSharedPtr<FUghReplay>& GetReplay() const { return Replay; }
	/**
	 * The replay a ghost flies in `Level` of the mode of `Players`: the best one kept, made by the same logic and data
	 * (another would fly elsewhere); none when the ghost is off (`bOn`) or there is no best.
	 */
	static TSharedPtr<FUghReplay> Of(const FUghReplays& Replays, int32 Players, int32 Level, bool bOn);

	/**
	 * A frame after the game's steps: `PlaySteps` the steps of the play of its attempt so far (FUghSimulation::
	 * GetPlaySteps; 0 not in the play). In the play the ghost makes as many of its own; out of it, it gets ready for the
	 * next play within `Budget` seconds.
	 */
	void Follow(int32 PlaySteps, double Budget = AlignBudget);

	/** It flies now: its copters are to be shown (between Previous and Current, the game's Alpha). */
	bool IsShown() const { return Stage == EStage::Flying && Current.copter_count > 0; }
	/** Ready at the start of its play, waiting for the game's. */
	bool IsReady() const { return Stage == EStage::Ready; }
	const ugh_logic_view& GetPrevious() const { return Previous; }
	const ugh_logic_view& GetCurrent() const { return Current; }
	/** The steps of its play made (in step with the game's). */
	int32 GetSteps() const { return Steps; }

private:
	/** None: no replay or not begun; Aligning: run to its play's start; Ready there; Flying with the game; Over. */
	enum class EStage : uint8 { None, Aligning, Ready, Flying, Over };
	static constexpr int32 AlignStepsAtOnce = 16;

	/** Resumed at the replay's start. */
	void BeginAlign();
	/** Run on towards the first step of its last attempt's play, for `Budget` seconds at most. */
	void AlignSome(double Budget);
	/** One step of its logic; false when its run is over (its play ended, the replay over). */
	bool Step();

	ugh_logic* Logic = nullptr;
	TSharedPtr<FUghReplay> Replay;
	ugh_logic_view Previous;
	ugh_logic_view Current;
	EStage Stage = EStage::None;
	int32 Steps = 0;
	int32 Attempts = 0;   // the replay's attempts begun (aligning)
};
