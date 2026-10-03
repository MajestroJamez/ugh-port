// A passenger with a route in the water.
#pragma once

#include "data/AnimatedPassengerKind.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

struct RoutePassengerKind;

/** A passenger with a route in the water: how long it stays afloat and whether a copter can rescue it. */
struct SwimmerKind : AnimatedPassengerKind {
    units::Int16 swimTime;                      // frames it stays afloat
    bool rescuable = true;                      // a copter on the water can pick it up
    const RoutePassengerKind* land = nullptr;   // the same passenger on land
};

}  // namespace ugh::data
