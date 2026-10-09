// Checks the replays of a level (.ughr) against a golden replay.
#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "check/ReplayCheck.hpp"
#include "check/ReplayReport.hpp"
#include "data/GameData.hpp"
#include "events/EventListener.hpp"
#include "game/AttemptStart.hpp"
#include "keyboard/KeyBinding.hpp"
#include "record/Recorder.hpp"
#include "record/Recording.hpp"

namespace ugh::check {

/**
 * The replays of a level (`record/`, the `.ughr` of the game) against a golden replay "UGR 1": the golden replay is
 * played (ReplayCheck) with the recorder of the levels listening; every attempt it starts after the test pilot's last
 * intervention (a replay of a level has only keys) is cut out of its level's recording as a replay of its own, written
 * as text and read back (the same), then played on a new game resumed at its start (`record::Playback`) - tick by tick
 * it must give the golden replay's state. Only the bonus items left by the attempt before may differ before the play
 * (they are cleared before it; a resumed game has none).
 */
class LevelReplayCheck : private ReplayCheck::Observer, private events::EventListener {
public:
    LevelReplayCheck(const data::GameData& data, const std::vector<keyboard::KeyBinding>& keys, ReplayReport& report);

    /** Plays the file into the report; how many replays of attempts it played: `played()`. */
    void run(const std::string& path);
    int played() const { return static_cast<int>(cut_.size()); }
    /** A replay of an attempt it played: the golden replay's tick it starts at, its points, as a player shares it. */
    struct Shared {
        long long tick;
        uint32_t points;
        std::string text;
    };
    /** The replays of attempts it played. */
    std::vector<Shared> texts() const;

private:
    /** An attempt of the golden replay: the tick its step started it in, and what it started from. */
    struct Attempt {
        long long tick;
        game::AttemptStart start;
    };
    /** A replay of a level the recorder made, and the tick its first attempt started in. */
    struct Level {
        long long tick;
        record::Recording recording;
    };
    /** A replay cut out to start at an attempt. */
    struct Cut {
        long long tick;
        record::Recording recording;
    };

    const data::GameData& data_;
    const std::vector<keyboard::KeyBinding>& keys_;
    ReplayReport& report_;
    record::Recorder recorder_;
    std::vector<Attempt> attempts_;
    std::vector<Level> levels_;
    std::vector<Cut> cut_;
    long long lastIntervention_ = -1;   // the tick the test pilot last set something after
    bool attemptStarted_ = false;

    // ReplayCheck::Observer
    void started(game::Game& game) override;
    void key(int player, input::PlayerKey key, bool pressed) override;
    void menuKey(input::MenuKey key) override;
    void intervened(long long after) override { lastIntervention_ = after; }
    void stepped(const game::Game& game, game::GameResult result, long long tick) override;
    // events::EventListener
    void onEvent(const events::Event& event) override;

    /** Keeps the recorder's recording of a level that ended (once). */
    void keep(const record::Recording& recording);
    /** The replays of the attempts after the last intervention, cut out of the levels' recordings. */
    void cutAttempts();
    /** Plays the cut replays along the golden replay again. */
    void playCut(const std::string& path);
};

}  // namespace ugh::check
