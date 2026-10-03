// Where a passenger is on its route.
#pragma once

#include "data/levels/Route.hpp"
#include "units/Countdown.hpp"
#include "world/Level.hpp"
#include "world/scenery/Pad.hpp"

namespace ugh::passengers::route {

/**
 * Where a passenger is on its route (`data::levels::Route`) in the level being played: the stop it is on, its pads, and
 * the delay before it comes out of the door there.
 */
class RouteProgress {
public:
    RouteProgress(const data::levels::Route& route, world::Level& level) : route_(&route), level_(&level) {}

    /** The stop it is on (the stop count when the route is done). */
    int stop() const { return stop_; }
    bool finished() const { return stop_ >= static_cast<int>(route_->stops.size()); }
    /** The pad of the stop where it is picked up, and the one it wants to go to. */
    world::scenery::Pad& pickupPad() const { return level_->pad(route_->stops[stop_].pickupPad); }
    world::scenery::Pad& targetPad() const { return level_->pad(route_->stops[stop_].targetPad); }
    /** On to the next stop. */
    void next() { stop_++; }

    /** The stop starts: the passenger comes out after the delay of the stop. */
    void startArrival() { arrival_.start(route_->stops[stop_].delay); }
    /** One frame of the delay before it comes out; true when it is over (a delay of 0 is none). */
    bool arrivalDue() { return arrival_.tickToZero(); }
    int arrivalDelay() const { return arrival_.remaining(); }

private:
    const data::levels::Route* route_;
    world::Level* level_;
    int stop_ = 0;
    units::Countdown arrival_;
};

}  // namespace ugh::passengers::route
