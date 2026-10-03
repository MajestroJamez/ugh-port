// A kind of passenger with a route.
#pragma once

#include "data/kinds/AnimatedPassengerKind.hpp"
#include "data/kinds/Animation.hpp"

namespace ugh::data::kinds {

struct SwimmerKind;

/** A kind of passenger with a route, on land: it comes out of a door and goes into one; `swimmer` in the water. */
struct RoutePassengerKind : AnimatedPassengerKind {
    const Animation* comingOut = nullptr;   // out of the door
    const Animation* goingIn = nullptr;     // into the door
    int look = 0;                           // who sits in the copter
    const SwimmerKind* swimmer = nullptr;   // the same passenger in the water
};

}  // namespace ugh::data::kinds
