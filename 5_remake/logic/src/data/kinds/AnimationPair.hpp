// An animation for each direction.
#pragma once

#include "data/kinds/Animation.hpp"
#include "data/kinds/Facing.hpp"

namespace ugh::data::kinds {

/** The left and the right variant of an animation (a walker walking, a passenger walking). */
struct AnimationPair {
    const Animation* left = nullptr;
    const Animation* right = nullptr;

    /** The variant of an entity that faces `side`. */
    const Animation& towards(Facing side) const { return side == Facing::Right ? *right : *left; }
};

}  // namespace ugh::data::kinds
