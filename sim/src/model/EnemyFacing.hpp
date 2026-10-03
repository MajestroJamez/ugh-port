// A word of the original an enemy uses for two things.
#pragma once

#include "core/Word.hpp"

namespace ugh::model {

/**
 * The enemy's word 2d57: the direction a walker faces (0 left, 2 right), or the player a flyer hunts (0 or 1).
 */
class EnemyFacing {
public:
    EnemyFacing() = default;
    explicit EnemyFacing(core::Word word) : word_(word) {}

    // ------------------------------------------------------------ a walker
    void faceLeft() { word_ = 0; }
    void faceRight() { word_ = 2; }
    void turnAround() { word_ = word_ ^ 2; }
    bool right() const { return word_ != 0; }
    /** -1 facing left, 1 facing right. */
    core::Word direction() const { return word_ - 1; }

    // ------------------------------------------------------------ a flyer
    /** The first flight goes for player 0. */
    void beforeFirstFlight() { word_ = 1; }
    /** 113b:23ea - the other player is the next target (player 0 without a second player). */
    int targetNextPlayer(core::Word players) {
        core::Word target = word_ ^ 1;
        if (target >= players) target = 0;
        word_ = target;
        return target.value();
    }
    int target() const { return word_.value(); }

    core::Word word() const { return word_; }

private:
    core::Word word_;
};

}  // namespace ugh::model
