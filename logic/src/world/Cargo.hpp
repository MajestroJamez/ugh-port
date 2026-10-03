// Who a copter carries.
#pragma once

#include <optional>

#include "units/Int16.hpp"

namespace ugh::world {

/** Who a copter carries: a passenger of a route inside, or the standing passenger hanging below. */
struct Cargo {
    units::Int16 look;                    // who sits in the copter (PassengerKind::look)
    std::optional<units::Int16> destination;   // the number of the pad it wants to go to; none: hanging below
    units::Int16 fareMin;                 // the fare drops to this (a passenger of a route)
};

}  // namespace ugh::world
