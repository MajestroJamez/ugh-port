// What lasts from one level to the next.
#pragma once

#include <cstdint>

#include "data/Difficulty.hpp"
#include "data/Rules.hpp"
#include "world/RandomNumbers.hpp"
#include "world/Score.hpp"

namespace ugh::testing {
class TestPilot;   // the test pilot of the replays (5_remake/logic/testing)
}

namespace ugh::world {

/** The game being played: the players, the difficulty, the level number, lives, score, multiplier, random numbers. */
class Session {
public:
    static constexpr int START_LIVES = 3;
    static constexpr int MAX_LIVES = 99;

    Session(const data::Rules& rules, int players, data::Difficulty difficulty, int firstLevel, RandomNumbers random)
        : rules_(&rules),
          players_(players),
          difficulty_(difficulty),
          levelNumber_(firstLevel),
          score_(rules.multiplierLimit(difficulty)),
          random_(random) {}

    /** The game starts: 3 lives, multiplier 1, score 0. */
    void startGame();

    int players() const { return players_; }
    data::Difficulty difficulty() const { return difficulty_; }
    int levelNumber() const { return levelNumber_; }
    int lives() const { return lives_; }
    /** The points and the multiplier. */
    Score& score() { return score_; }
    const Score& score() const { return score_; }

    /** The level is done: on to the next one; false when there is none. */
    bool nextLevel(int levelCount);

    /** The attempt failed: a life less and the multiplier back to 1; false when no life is left. */
    bool loseLife();

    /** Esc gives the game up: the attempt that ends is the last one. */
    void giveUp() { lives_ = 0; }

    /** A life bonus item: more lives, at most MAX_LIVES. */
    void addLives(int amount);

    /** A bounce this hard crashes a copter on this difficulty. */
    int crashLimit() const { return rules_->crashLimit(difficulty_); }

    RandomNumbers& random() { return random_; }
    const RandomNumbers& random() const { return random_; }

private:
    friend class testing::TestPilot;   // it keeps the lives up

    const data::Rules* rules_;
    int players_ = 0;
    data::Difficulty difficulty_;
    int levelNumber_ = 0;
    int lives_ = 0;
    Score score_;
    RandomNumbers random_;
};

}  // namespace ugh::world
