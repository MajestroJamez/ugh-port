// Where a passenger is on its route.
#pragma once

#include "data/Route.hpp"
#include "units/Int16.hpp"

namespace ugh::passengers::route {

/** Where a passenger is on its route: the stop it is on and the delay before it comes out of the door there. */
class Route {
public:
    explicit Route(const data::Route& route) : route_(&route) {}

    /** The stop it is on (the stop count when the route is done). */
    int stop() const { return stop_; }
    bool finished() const { return stop_ >= static_cast<int>(route_->stops.size()); }
    int pickupPad() const { return route_->stops[stop_].pickupPad; }
    int targetPad() const { return route_->stops[stop_].targetPad; }
    /** On to the next stop. */
    void next() { stop_++; }

    /** The stop starts: the passenger comes out after the delay of the stop. */
    void startArrival() { arrivalDelay_ = route_->stops[stop_].delay; }
    /** One frame of the delay before it comes out; true when it is over (a delay of 0 is none). */
    bool arrivalDue() {
        if (arrivalDelay_ == 0) return true;
        arrivalDelay_ -= 1;
        return arrivalDelay_ == 0;
    }
    units::Int16 arrivalDelay() const { return arrivalDelay_; }

private:
    const data::Route* route_;
    int stop_ = 0;
    units::Int16 arrivalDelay_;
};

}  // namespace ugh::passengers::route
