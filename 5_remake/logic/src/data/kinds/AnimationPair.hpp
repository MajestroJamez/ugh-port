// An animation for each direction.
#pragma once

#include "data/kinds/Animation.hpp"

namespace ugh::data::kinds {

/** The left and the right variant of an animation (a walker walking, a passenger walking). */
struct AnimationPair {
    const Animation* left = nullptr;
    const Animation* right = nullptr;

    const Animation& towards(bool toTheRight) const { return toTheRight ? *right : *left; }
};

}  // namespace ugh::data::kinds
