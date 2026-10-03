#include "passengers/swimming/Swimming.hpp"

#include "passengers/swimming/Sinking.hpp"
#include "passengers/swimming/SwimCalling.hpp"

namespace ugh::passengers {

const Swimming Swimming::instance{};

/** 113b:1f24 - afloat for the swim time of its kind. */
void Swimming::enter(model::Passenger& passenger, model::Level&) const {
    passenger.restartAnimation();
    passenger.timer().startSwimTime(passenger.kind().swimTime);
}

/** 113b:1f43 - swims; a copter floating still nearby rescues it (all kinds but 77fe), else it drowns in time. */
void Swimming::update(model::Passenger& passenger, model::Level& level) const {
    passenger.animate();
    passenger.show(*passenger.kind().waving);
    passenger.floatOnSurface(level.water().row());
    if (passenger.kind().rescuable && level.copterOnWater(true, true) != model::Level::NONE) {
        passenger.changeState(SwimCalling::instance, level);
        return;
    }
    if (passenger.timer().swimTimeUp()) passenger.changeState(Sinking::instance, level);
}

}  // namespace ugh::passengers
