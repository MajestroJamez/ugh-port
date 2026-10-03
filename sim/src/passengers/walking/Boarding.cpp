#include "passengers/walking/Boarding.hpp"

#include "passengers/walking/Impatient.hpp"
#include "passengers/walking/Riding.hpp"

namespace ugh::passengers {

const Boarding Boarding::instance{};

/** 113b:18c8 - the bubble goes (also on the water: SwimBoarding). */
void Boarding::enter(model::Passenger& passenger, model::Level&) const {
    passenger.restartAnimation();
    passenger.hideBubble();
}

/** 113b:18e6 - walks to the landed copter and boards it; waves impatiently when it took off. */
void Boarding::update(model::Passenger& passenger, model::Level& level) const {
    if (fellIntoWater(passenger, level)) return;
    if (knockedIntoWater(passenger, level)) return;
    int copter = level.copterLandedOn(passenger.pickupPad());
    if (copter == model::Level::NONE) {
        passenger.changeState(Impatient::instance, level);
        return;
    }
    if (passenger.walkTowards(level.copter(copter))) Riding::board(passenger, copter, level);
}

}  // namespace ugh::passengers
