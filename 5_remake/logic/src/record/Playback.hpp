// A replay played on a game.
#pragma once

#include <cstddef>
#include <span>
#include <utility>

#include "game/Game.hpp"
#include "record/Recording.hpp"

namespace ugh::record {

/**
 * Plays a recording on a game: the game resumed at the recording's start (`game::Game::resume`), before each step the
 * inputs that came before it, as many steps as the recording has. Whoever steps the game gives it the inputs `due()`
 * and calls `stepped()` after each step.
 */
class Playback {
public:
    explicit Playback(Recording recording) : recording_(std::move(recording)) {}

    const Recording& recording() const { return recording_; }

    /** Resumes `game` at the recording's start; false when the game refuses it (a level the data has not). */
    bool start(game::Game& game);

    /** The steps made so far. */
    int stepsMade() const { return next_; }
    /** Every step of the recording is made. */
    bool over() const { return next_ >= recording_.steps; }
    /** The inputs before the next step, in their order. */
    std::span<const Input> due() const;
    /** The next step was made (after its inputs). */
    void stepped();

private:
    Recording recording_;
    int next_ = 0;          // the next step
    size_t given_ = 0;      // the inputs given so far

    size_t dueEnd() const;
};

}  // namespace ugh::record
