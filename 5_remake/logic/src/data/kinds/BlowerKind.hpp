// The blower.
#pragma once

#include "data/kinds/Animation.hpp"
#include "data/kinds/Box.hpp"

namespace ugh::data::kinds {

/** The blower: it blows copters in front of it to and fro. */
struct BlowerKind {
    Box box;
    const Animation* blowing = nullptr;
    int stunnedSprite = 0;
    int score = 0;   // for stunning it with a passenger
};

}  // namespace ugh::data::kinds
