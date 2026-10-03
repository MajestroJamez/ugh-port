// The triceratops.
#pragma once

#include "data/AnimationPair.hpp"
#include "data/Box.hpp"

namespace ugh::data {

/** The walker (triceratops): it walks on its pad and charges at a copter that lands there. */
struct WalkerKind {
    Box box;
    AnimationPair walk, watch, charge, recover, stunned;
    int score = 0;   // for stunning it with a passenger
};

}  // namespace ugh::data
