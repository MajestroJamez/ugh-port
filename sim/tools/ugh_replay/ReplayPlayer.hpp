// Plays a golden replay on the core.
#pragma once

#include <string>
#include <vector>

#include "ReplayFile.hpp"
#include "ReplayReport.hpp"
#include "TwinCores.hpp"

namespace ugh::tool {

/**
 * Checks the core against a replay, in one of two modes.
 *
 * The whole game (default): the core starts from the state of tick 0 (the start of a new game) and runs on its own
 * (ugh_sim_step), getting only the keys and the injections (I lines) of the recording; after every tick all fields
 * are compared with the recording. A field the core does not know (memory left by the screens before the game,
 * e.g. passengers of the attract mode) is taken over the first time the recording shows it; a mismatch is reported
 * and the recorded value taken over, so one error does not hide the next ones.
 *
 * Each transition (--each): every transition on its own, from the recorded state before the tick (T line, I line,
 * keys); the stages the core does not have are undone on the expected state (B lines):
 *   new game     start -> betweenLevels             game.*
 *   level start  betweenLevels / play -> caption    game.*, copter.* and the groups of the stages the core has
 *                                                  (pad.* with passenger.*, object.*, bonus.*; from play: its last
 *                                                  frame and the level end first)
 *   play frame   play -> play                       the same (every field must be known)
 */
class ReplayPlayer {
public:
    enum class Mode { WholeGame, EachTransition };

    ReplayPlayer(TwinCores& cores, ReplayReport& report) : cores_(cores), report_(report) {}

    /** Plays the file; false when it cannot be read (the reason is on stderr). */
    bool play(const std::string& path, Mode mode);

private:
    /** What to do with a recorded field the core does not know. */
    enum class Unknown { Count, Error, TakeOver };

    static constexpr long long NO_TICK = -1;

    TwinCores& cores_;
    ReplayReport& report_;

    void playWhole(const Tick& tick, bool last);
    void playTransition(const Tick& before, const Tick& after);
    void checkLevelStart(const Tick& before, const Tick& after);

    void load(const Tick& tick);
    void set(const std::string& name, const std::string& value);
    /** A problem at the tick (NO_TICK when it belongs to none). */
    void problem(long long tick, const std::string& what, const std::string& text);
    void takeProblems();
    void compare(const std::string& what, const Tick& tick, const Fields& expected,
                 const std::vector<const char*>& groups, Unknown unknown, bool takeOverMismatches);

    static std::vector<const char*> groups();
    static Fields undoMissingStages(const Tick& tick);
    static std::string general(const std::string& name);
    static const std::string& phase(const Tick& tick);
};

}  // namespace ugh::tool
