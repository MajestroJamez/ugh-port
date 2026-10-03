// What lasts from one level to the next.
#pragma once

#include "data/Difficulty.hpp"
#include "data/Rules.hpp"
#include "world/session/Lives.hpp"
#include "world/session/RandomNumbers.hpp"
#include "world/session/Score.hpp"

namespace ugh::world::session {

/** The game being played: the players, the difficulty, the level number, lives, score, multiplier, random numbers. */
class Session {
public:
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
    Lives& lives() { return lives_; }
    const Lives& lives() const { return lives_; }
    /** The points and the multiplier. */
    Score& score() { return score_; }
    const Score& score() const { return score_; }

    /** The level is done: on to the next one; false when there is none. */
    bool nextLevel(int levelCount);

    /** The attempt failed: a life less and the multiplier back to 1; false when no life is left. */
    bool loseLife();

    /** A bounce this hard crashes a copter on this difficulty. */
    int crashLimit() const { return rules_->crashLimit(difficulty_); }

    RandomNumbers& random() { return random_; }
    const RandomNumbers& random() const { return random_; }

private:
    const data::Rules* rules_;
    int players_ = 0;
    data::Difficulty difficulty_;
    int levelNumber_ = 0;
    Lives lives_;
    Score score_;
    RandomNumbers random_;
};

}  // namespace ugh::world::session
