// A word of the original an enemy uses for two things.
#pragma once

#include "core/Word.hpp"
#include "model/SharedWord.hpp"

namespace ugh::model {

/**
 * The enemy's word 2d57: the direction a walker faces (0 left, 2 right), or the player a flyer hunts (0 or 1).
 */
class EnemyFacing : public SharedWord {
public:
    EnemyFacing() = default;
    explicit EnemyFacing(core::Word word) : SharedWord(word) {}

    /** The walker's directions: the offsets of its two-word tables (left, right). */
    static constexpr core::Word LEFT = 0, RIGHT = 2;

    // ------------------------------------------------------------ a walker
    void faceLeft() { word_ = LEFT; wrote("facing"); }
    void faceRight() { word_ = RIGHT; wrote("facing"); }
    void turnAround() { reading("EnemyFacing", "facing"); wrote("facing"); word_ = word_ ^ RIGHT; }
    bool right() const { reading("EnemyFacing", "facing"); return word_ != LEFT; }
    /** -1 facing left, 1 facing right. */
    core::Word direction() const { reading("EnemyFacing", "facing"); return word_ - 1; }

    // ------------------------------------------------------------ a flyer
    /** The first flight goes for player 0. */
    void beforeFirstFlight() { word_ = 1; wrote("target"); }
    /** 113b:23ea - the other player is the next target (player 0 without a second player). */
    int targetNextPlayer(core::Word players) {
        reading("EnemyFacing", "target");
        wrote("target");
        core::Word target = word_ ^ 1;
        if (target >= players) target = 0;
        word_ = target;
        return target.value();
    }
    int target() const { reading("EnemyFacing", "target"); return word_.value(); }
};

}  // namespace ugh::model
