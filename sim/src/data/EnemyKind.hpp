// A kind of enemy.
#pragma once

#include <cstdint>

#include "core/Word.hpp"
#include "data/AnimationPair.hpp"
#include "data/Box.hpp"

namespace ugh::data {

/** A kind of enemy (descriptor at 7630 flyer, 766c walker, 76a8 blower, 76e4 tree, 0x3c bytes each). */
struct EnemyKind {
    /** The pterodactyl, the triceratops walking on a pad, the blower, the tree that drops bonus items. */
    enum class Type { Flyer, Walker, Blower, Tree };

    uint16_t origin = 0;       // the offset in the original's data
    Type type = Type::Walker;
    Box box;
    // the animations of the descriptor (+22 .. +38); every kind uses some of them
    AnimationPair moving;      // +22 / +24: flyer flight, walker walk, tree sway (left)
    AnimationPair charging;    // +26 / +28: walker charge, blower blowing (left)
    AnimationPair recovering;  // +2a / +2c: walker
    AnimationPair watching;    // +2e / +30: walker
    AnimationPair stunned;     // +36 / +38: flyer hit, walker and blower stunned
    core::Word score;          // +3a: for stunning it with a passenger
};

}  // namespace ugh::data
