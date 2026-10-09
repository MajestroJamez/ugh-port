// Checks the logic against a golden replay.
#pragma once

#include <deque>
#include <string>
#include <vector>

#include "check/ReplayFile.hpp"
#include "check/ReplayReport.hpp"
#include "data/GameData.hpp"
#include "game/Game.hpp"
#include "keyboard/KeyBinding.hpp"
#include "keyboard/PcKeyboard.hpp"

namespace ugh::check {

/**
 * Plays a golden replay "UGR 1" on the logic: tick 0 gives the new game's settings, every later tick is one step of
 * the logic with the recorded scancodes and the test pilot's interventions (I lines, through `testing::TestPilot`), and after every
 * tick the state of the logic (StateWriter) must be the recorded one: the same fields with the same values.
 */
class ReplayCheck {
public:
    /** What to check. */
    using Tick = ReplayFile::Tick;

    /** Who wants to see the game as the replay plays it (LevelReplayCheck: the replays of a level). */
    class Observer {
    public:
        virtual ~Observer() = default;
        /** The new game of tick 0. */
        virtual void started(game::Game& game) = 0;
        /** An input the keyboard gave the game, after tick `after`. */
        virtual void key(int player, input::PlayerKey key, bool pressed) = 0;
        virtual void menuKey(input::MenuKey key) = 0;
        /** The test pilot set something after tick `after`. */
        virtual void intervened(long long after) = 0;
        /** The step of tick `tick` was made and returned `result`. */
        virtual void stepped(const game::Game& game, game::GameResult result, long long tick) = 0;
    };

    struct Options {
        bool continueAfterMismatch = false;   // count all mismatches (for statistics) instead of stopping at the first
        Observer* observer = nullptr;
    };

    ReplayCheck(const data::GameData& data, const std::vector<keyboard::KeyBinding>& keys, const Options& options,
                ReplayReport& report)
        : data_(data), keys_(keys), options_(options), report_(report) {}

    /** Plays the file into the report (a file that cannot be read is a problem of the report too). */
    void run(const std::string& path);

private:
    static constexpr size_t RECENT_TICKS = 5;

    const data::GameData& data_;
    const std::vector<keyboard::KeyBinding>& keys_;
    const Options& options_;
    ReplayReport& report_;
    std::deque<std::string> recent_;   // the scancodes of the last ticks, for the report

    bool start(game::Game& game, const Tick& tick);
    /** Compares; false at a mismatch that stops the check. */
    bool compare(const game::Game& game, const Tick& tick);
    void apply(game::Game& game, keyboard::PcKeyboard& keyboard, const Tick& tick);
    void intervene(game::Game& game, const Tick& tick);
    void diagnostics(game::Game& game, long long tick);

    /** A whole number in `text`; false when it is not one. */
    static bool number(const std::string& text, int& out, int base = 10);
};

}  // namespace ugh::check
