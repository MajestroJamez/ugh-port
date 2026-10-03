// What lasts from one level to the next.
#pragma once

#include <cstdint>

#include "data/Difficulty.hpp"
#include "data/Rules.hpp"
#include "units/Int16.hpp"
#include "world/RandomNumbers.hpp"

namespace ugh::world {

/** The game being played: the players, the difficulty, the level number, lives, score, multiplier, random numbers. */
class Session {
public:
    static constexpr int START_LIVES = 3;
    static constexpr int MAX_LIVES = 99;

    Session(const data::Rules& rules, int players, data::Difficulty difficulty, int firstLevel, RandomNumbers random)
        : rules_(&rules), players_(players), difficulty_(difficulty), levelNumber_(firstLevel), random_(random) {}

    /** The game starts: 3 lives, multiplier 1, score 0. */
    void startGame();

    int players() const { return players_; }
    data::Difficulty difficulty() const { return difficulty_; }
    int levelNumber() const { return levelNumber_; }
    int lives() const { return lives_; }
    int multiplier() const { return multiplier_; }
    uint32_t score() const { return score_; }

    /** The level is done: on to the next one; false when there is none. */
    bool nextLevel(int levelCount);

    /** The attempt failed: a life less and the multiplier back to 1; false when no life is left. */
    bool loseLife();

    /** Esc gives the game up: the attempt that ends is the last one. */
    void giveUp() { lives_ = 0; }

    /** The test pilot of the replays sets the lives (Cheats only). */
    void setLivesByTestPilot(int lives) { lives_ = lives; }

    /** A life bonus item: more lives, at most MAX_LIVES. */
    void addLives(units::Int16 amount);

    void addScore(uint32_t points) { score_ += points; }

    /** The score multiplier can go higher on this difficulty. */
    bool multiplierBelowLimit() const { return multiplier_ < rules_->multiplierLimit(difficulty_).value(); }
    /** A multiplier bonus item. */
    void raiseMultiplier();

    /** A bounce this hard crashes a copter on this difficulty. */
    units::Int16 crashLimit() const { return rules_->crashLimit(difficulty_); }

    RandomNumbers& random() { return random_; }
    const RandomNumbers& random() const { return random_; }

private:
    const data::Rules* rules_;
    int players_;
    data::Difficulty difficulty_;
    int levelNumber_;
    int lives_ = 0;
    int multiplier_ = 0;
    uint32_t score_ = 0;
    RandomNumbers random_;
};

}  // namespace ugh::world
