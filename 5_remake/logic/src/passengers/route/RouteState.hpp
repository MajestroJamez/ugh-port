// A state of a passenger with a route.
#pragma once

#include "passengers/PassengerContext.hpp"
#include "state/State.hpp"

namespace ugh::passengers::route {

class RoutePassenger;

/**
 * A state of a passenger with a route: out of the door of its pickup pad, wait, call a copter that lands
 * there, ride, pay, walk to the door of the target pad, next stop; in the water: splash, swim, call a copter on the
 * water, or sink. The states are stateless singletons; the passenger holds the data.
 */
using RouteState = state::State<RoutePassenger, PassengerContext>;

}  // namespace ugh::passengers::route
