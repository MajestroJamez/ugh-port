// The route of a passenger.
#pragma once

#include <vector>

#include "units/Int16.hpp"

namespace ugh::data {

/** The route of a passenger: its stops, each from one pad to another after a delay. */
struct Route {
    /** A stop: the passenger comes out at the pickup pad after the delay and wants to go to the target pad. */
    struct Stop {
        int pickupPad = 0;
        units::Int16 delay;   // frames
        int targetPad = 0;
    };

    std::vector<Stop> stops;
};

}  // namespace ugh::data
