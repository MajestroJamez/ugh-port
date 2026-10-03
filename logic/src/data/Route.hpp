// The route of a passenger.
#pragma once

#include <utility>
#include <vector>

#include "units/Int16.hpp"

namespace ugh::data {

/** The route of a passenger: its stops, each from one pad to another after a delay. */
class Route {
public:
    /** A stop of the route: the passenger comes out at the pickup pad after the delay and wants to go to the target pad. */
    struct Stop {
        int pickupPad = 0;
        units::Int16 delay;   // frames
        int targetPad = 0;
    };

    explicit Route(std::vector<Stop> stops) : stops_(std::move(stops)) {}

    int stopCount() const { return static_cast<int>(stops_.size()); }
    const Stop& stop(int index) const { return stops_[index]; }

private:
    std::vector<Stop> stops_;
};

}  // namespace ugh::data
