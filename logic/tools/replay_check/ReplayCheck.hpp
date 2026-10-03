// Checks the logic against a golden replay.
#pragma once

#include <deque>
#include <string>
#include <vector>

#include "ReplayFile.hpp"
#include "ReplayReport.hpp"
#include "data/GameData.hpp"
#include "game/Game.hpp"

namespace ugh::tool {

/** What to check. */
struct CheckOptions {
    bool continueAfterMismatch = false;   // count all mismatches (for statistics) instead of stopping at the first
    std::vector<std::string> only;        // field prefixes to compare ("game.", "copter."); empty: all
    std::vector<std::string> skip;        // fields not compared, N for any index ("copter.N.cargoLook")
    std::string untilField, untilValue;   // stop before the first tick where the field has (or, `untilNot`, has not)
    bool untilNot = false;                //   this value
};

/**
 * Plays a golden replay "UGR 1" on the logic: tick 0 gives the new game's settings, every later tick is one step of
 * the logic with the recorded scancodes and the test pilot's interventions (I lines, through Cheats), and after every
 * tick the state of the logic (StateWriter) must be the recorded one: the same fields with the same values.
 */
class ReplayCheck {
public:
    ReplayCheck(const data::GameData& data, const CheckOptions& options, ReplayReport& report)
        : data_(data), options_(options), report_(report) {}

    /** Plays the file; false when it cannot be read (the reason is on stderr). */
    bool run(const std::string& path);

private:
    static constexpr size_t RECENT_TICKS = 5;

    const data::GameData& data_;
    const CheckOptions& options_;
    ReplayReport& report_;
    std::deque<std::string> recent_;   // the scancodes of the last ticks, for the report

    bool start(game::Game& game, const Tick& tick);
    /** Compares; false at a mismatch that stops the check. */
    bool compare(const game::Game& game, const Tick& tick);
    void apply(game::Game& game, const Tick& tick);
    bool compared(const std::string& field) const;
    bool stopsAt(const Tick& tick) const;
    void diagnostics(game::Game& game, long long tick);

    static std::string general(const std::string& field);
};

}  // namespace ugh::tool
