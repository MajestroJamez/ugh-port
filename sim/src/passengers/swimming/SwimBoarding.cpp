#include "passengers/swimming/SwimBoarding.hpp"

#include "passengers/swimming/SwimWaving.hpp"
#include "passengers/walking/Boarding.hpp"
#include "passengers/walking/Riding.hpp"

namespace ugh::passengers {

const SwimBoarding SwimBoarding::instance{};

/** 113b:18c8 - the same as boarding on a pad. */
void SwimBoarding::enter(model::Passenger& passenger, model::Level& level) const {
    Boarding::instance.enter(passenger, level);
}

/** 113b:20c1 - swims to the copter on the water and boards it; waves when it is gone or full. */
void SwimBoarding::update(model::Passenger& passenger, model::Level& level) const {
    passenger.floatOnSurface(level.water().row());
    int copter = level.copterOnWater(true, false);
    if (copter == model::Level::NONE) {
        passenger.changeState(SwimWaving::instance, level);
        return;
    }
    if (passenger.walkTowards(level.copter(copter))) Riding::board(passenger, copter, level);
}

}  // namespace ugh::passengers
