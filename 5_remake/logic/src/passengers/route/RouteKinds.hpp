// The kind of a passenger with a route, on land and in the water.
#pragma once

#include "data/AnimatedPassengerKind.hpp"
#include "data/RoutePassengerKind.hpp"
#include "data/SwimmerKind.hpp"

namespace ugh::passengers::route {

/** The kind of a passenger with a route: its kind on land, and the swimmer kind of it while it is in the water. */
class RouteKinds {
public:
    explicit RouteKinds(const data::RoutePassengerKind& land) : land_(&land) {}

    /** The kind as it is now. */
    const data::AnimatedPassengerKind& current() const {
        if (inWater_) return *land_->swimmer;
        return *land_;
    }
    const data::RoutePassengerKind& land() const { return *land_; }
    const data::SwimmerKind& swimmer() const { return *land_->swimmer; }

    bool inWater() const { return inWater_; }
    /** It falls into the water: a swimmer until a copter rescues it. */
    void intoWater() { inWater_ = true; }
    void outOfWater() { inWater_ = false; }

private:
    const data::RoutePassengerKind* land_;
    bool inWater_ = false;
};

}  // namespace ugh::passengers::route
