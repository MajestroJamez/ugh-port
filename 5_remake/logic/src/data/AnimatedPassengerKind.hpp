// A kind of a passenger with a route, on land or in the water.
#pragma once

#include "data/Animation.hpp"
#include "data/AnimationPair.hpp"
#include "data/PassengerKind.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** What a passenger with a route shows and pays, on land (`RoutePassengerKind`) and in the water (`SwimmerKind`). */
struct AnimatedPassengerKind : PassengerKind {
    const Animation* standing = nullptr;   // in the water: treading water
    const Animation* waving = nullptr;
    AnimationPair walking;                 // in the water: swimming
    units::Int16 animDelay;                // frames per animation frame
    units::Int16 fare, fareMin;            // the fare when boarding drops every frame of the ride to the minimum
};

}  // namespace ugh::data
