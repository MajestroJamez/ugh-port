// The blower.
#pragma once

#include "data/Animation.hpp"
#include "data/Box.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** The blower: it blows copters in front of it to and fro. */
struct BlowerKind {
    Box box;
    const Animation* blowing = nullptr;
    int stunnedSprite = 0;
    units::Int16 score;   // for stunning it with a passenger
};

}  // namespace ugh::data
