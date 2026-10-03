// A kind of a passenger with a route, on land or in the water.
#pragma once

#include "data/kinds/Animation.hpp"
#include "data/kinds/AnimationPair.hpp"
#include "data/kinds/PassengerKind.hpp"

namespace ugh::data::kinds {

/** What a passenger with a route shows and pays, on land (`RoutePassengerKind`) and in the water (`SwimmerKind`). */
struct AnimatedPassengerKind : PassengerKind {
    const Animation* standing = nullptr;   // in the water: treading water
    const Animation* waving = nullptr;
    AnimationPair walking;       // in the water: swimming
    int animDelay = 0;           // frames per animation frame
    int fare = 0, fareMin = 0;   // the fare when boarding drops every frame of the ride to the minimum
};

}  // namespace ugh::data::kinds
