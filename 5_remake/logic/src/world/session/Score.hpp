// The score of a game.
#pragma once

#include <cstdint>

namespace ugh::world::session {

/**
 * The score of a game and its multiplier: a quick delivery raises the multiplier up to a limit, a lost life resets
 * it.
 */
class Score {
public:
    explicit Score(int multiplierLimit) : multiplierLimit_(multiplierLimit) {}

    /** A new game: no points, multiplier 1. */
    void start() {
        points_ = 0;
        multiplier_ = 1;
    }
    uint32_t points() const { return points_; }
    void add(uint32_t points) { points_ += points; }

    int multiplier() const { return multiplier_; }
    bool multiplierBelowLimit() const { return multiplier_ < multiplierLimit_; }
    void raiseMultiplier() {
        if (multiplierBelowLimit()) multiplier_++;
    }
    /** A life is lost: the multiplier starts again. */
    void resetMultiplier() { multiplier_ = 1; }

private:
    int multiplierLimit_;
    uint32_t points_ = 0;
    int multiplier_ = 0;
};

}  // namespace ugh::world::session
