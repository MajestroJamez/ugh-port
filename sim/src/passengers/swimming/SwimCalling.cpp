#include "passengers/swimming/SwimCalling.hpp"

#include "passengers/swimming/SwimBoarding.hpp"
#include "passengers/swimming/SwimWaving.hpp"
#include "passengers/walking/Calling.hpp"

namespace ugh::passengers {

const SwimCalling SwimCalling::instance{};

/** 113b:16f6 - the same as calling on a pad. */
void SwimCalling::enter(model::Passenger& passenger, model::Level& level) const {
    Calling::instance.enter(passenger, level);
}

/** 113b:1fe2 - calls the copter on the water for a while, then swims to it; waves when it left. */
void SwimCalling::update(model::Passenger& passenger, model::Level& level) const {
    passenger.floatOnSurface(level.water().row());
    if (level.copterOnWater(false, false) == model::Level::NONE) {
        passenger.changeState(SwimWaving::instance, level);
        return;
    }
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.counter().tick()) passenger.changeState(SwimBoarding::instance, level);
}

}  // namespace ugh::passengers
