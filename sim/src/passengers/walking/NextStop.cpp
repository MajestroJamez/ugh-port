#include "passengers/walking/NextStop.hpp"

#include "passengers/Gone.hpp"
#include "passengers/walking/Arriving.hpp"

namespace ugh::passengers {

const NextStop NextStop::instance{};

/** 113b:149c - the stop goes from the pickup pad to the target pad after a delay. */
void NextStop::update(model::Passenger& passenger, model::Level& level) const {
    if (passenger.routeFinished()) {
        level.passengerFinished();
        passenger.continueIn(Gone::instance, level);
        return;
    }
    passenger.startStop();
    passenger.changeState(Arriving::instance, level);
}

}  // namespace ugh::passengers
