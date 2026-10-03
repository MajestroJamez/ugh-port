// The pterodactyl.
#pragma once

#include "data/AnimationPair.hpp"
#include "data/Box.hpp"

namespace ugh::data {

/** The flyer (pterodactyl): it hunts the copters across the screen. */
struct FlyerKind {
    Box box;
    AnimationPair flight;
    int hitSpriteLeft = 0, hitSpriteRight = 0;   // falling after a passenger hit it
    int score = 0;                               // for hitting it with a passenger
};

}  // namespace ugh::data
