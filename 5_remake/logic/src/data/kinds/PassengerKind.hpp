// What every kind of passenger has.
#pragma once

#include <string>

#include "data/kinds/Box.hpp"

namespace ugh::data::kinds {

/**
 * What every kind of passenger has: its name and its box. The kinds are `RoutePassengerKind` (a passenger with a
 * route on land), `SwimmerKind` (the same passenger in the water) and `StandingPassengerKind`.
 */
struct PassengerKind {
    std::string name;
    Box box;
};

}  // namespace ugh::data::kinds
