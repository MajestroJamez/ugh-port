// What survives from one level to the next.
#pragma once

#include <cstdint>

#include "core/Random.hpp"
#include "core/Word.hpp"
#include "data/GameData.hpp"

namespace ugh::model {

/** The game being played: the players, the difficulty, the level number, lives, score and its multiplier. */
class GameSession {
public:
    struct Snapshot {
        core::Word players = 1;       // 1 or 2 (team)
        core::Word difficulty = 1;    // 0 .. 2
        core::Word levelNumber = 0;   // from 0
        uint8_t lives = 0;
        uint8_t multiplier = 0;
        uint32_t score = 0;
    };

    explicit GameSession(const data::GameData& data) : data_(data) {}

    /** 113b:3961 - GameFlow.kt newGame: 3 lives, multiplier 1, score 0. */
    void newGame();

    /** The copters in play: both in the team mode. */
    int copterCount() const { return s_.players == 2 ? 2 : 1; }
    core::Word players() const { return s_.players; }

    /** The level being played as the data defines it; nullptr when there is no such level. */
    const data::LevelDefinition* levelDefinition() const;
    core::Word levelNumber() const { return s_.levelNumber; }

    /** 113b:0fa7 - the level is done: on to the next one; false when it was the last. */
    bool nextLevel();

    /** 113b:0fa7 - the attempt failed: one life less and the multiplier back to 1; false when the game is over. */
    bool loseLife();

    /** Esc gives the game up: no lives left after this attempt. */
    void giveUp() { s_.lives = 0; }

    /** 113b:2ca9 - a life bonus item: more lives, at most 99. */
    void addLives(core::Word amount);

    void addScore(uint32_t points) { s_.score += points; }
    uint8_t multiplier() const { return s_.multiplier; }
    /** The multiplier can go higher on this difficulty. */
    bool multiplierBelowLimit() const;
    /** 113b:2ca9 - a multiplier bonus item. */
    void raiseMultiplier();

    /** The impact that crashes a copter on this difficulty. */
    core::Word crashLimit() const { return data_.crashLimit(s_.difficulty.value()); }

    core::Random& random() { return random_; }
    const core::Random& random() const { return random_; }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    const data::GameData& data_;
    Snapshot s_;
    core::Random random_;
};

}  // namespace ugh::model
