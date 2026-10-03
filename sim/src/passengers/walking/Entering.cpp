#include "passengers/walking/Entering.hpp"

#include "passengers/walking/NextStop.hpp"

namespace ugh::passengers {

const Entering Entering::instance{};

/** 113b:1bbe */
void Entering::enter(model::Passenger& passenger, model::Level&) const { passenger.restartAnimation(); }

/** 113b:1bd6 - the door animation, then the next stop of the route. */
void Entering::update(model::Passenger& passenger, model::Level& level) const {
    if (!passenger.animate()) return;
    const data::Animation& door = *passenger.kind().entering;
    passenger.showFrameOf(door);
    if (!passenger.atLastFrameOf(door)) return;
    passenger.advanceRoute();
    passenger.continueIn(NextStop::instance, level);
}

}  // namespace ugh::passengers
