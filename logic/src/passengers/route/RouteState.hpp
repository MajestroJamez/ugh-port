// A state of a passenger with a route.
#pragma once

#include "passengers/PassengerContext.hpp"

namespace ugh::passengers::route {

class RoutePassenger;

/**
 * A state of a passenger with a route (State): out of the door of its pickup pad, wait, call a copter that lands
 * there, ride, pay, walk to the door of the target pad, next stop; in the water: splash, swim, call a copter on the
 * water, or sink. The states are stateless singletons; the passenger holds the data.
 */
class RouteState {
public:
    virtual ~RouteState() = default;

    /** The name of the state (the replays). */
    virtual const char* name() const = 0;
    /** What the passenger does when it gets into the state. */
    virtual void enter(RoutePassenger&, const PassengerContext&) const {}
    /** One frame in the state. */
    virtual void update(RoutePassenger& passenger, const PassengerContext& context) const = 0;
};

}  // namespace ugh::passengers::route
