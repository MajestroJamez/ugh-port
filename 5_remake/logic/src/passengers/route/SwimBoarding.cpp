#include "passengers/route/SwimBoarding.hpp"

#include "passengers/route/Riding.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/SwimWaving.hpp"

namespace ugh::passengers::route {

const SwimBoarding SwimBoarding::instance{};

/** The same as boarding on a pad. */
void SwimBoarding::enter(RoutePassenger& passenger, const PassengerContext&) const { passenger.startBoarding(); }

void SwimBoarding::update(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Level& level = context.level;
    passenger.floatOnSurface(level.water().row());
    world::copter::Copter* copter = level.copters().onWaterWithRoom(level.water());
    if (!copter) {
        passenger.changeState(SwimWaving::instance, context);
        return;
    }
    if (passenger.walkTowards(*copter)) Riding::board(passenger, *copter, context);
}

}  // namespace ugh::passengers::route
