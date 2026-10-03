// Who a copter carries.
#pragma once

#include <optional>

namespace ugh::world {

/** Who a copter carries: a passenger of a route inside, or the standing passenger hanging below. */
struct Cargo {
    int look = 0;                     // who sits in the copter (RoutePassengerKind::look)
    std::optional<int> destination;   // the number of the pad it wants to go to; none: hanging below
    int fareMin = 0;                  // the fare drops to this (a passenger of a route)
};

}  // namespace ugh::world
