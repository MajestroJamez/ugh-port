#include "passengers/route/SwimBoarding.hpp"

#include "passengers/route/Boarding.hpp"
#include "passengers/route/Riding.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/SwimWaving.hpp"

namespace ugh::passengers::route {

const SwimBoarding SwimBoarding::instance{};

/** The same as boarding on a pad. */
void SwimBoarding::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    Boarding::instance.enter(passenger, context);
}

void SwimBoarding::update(RoutePassenger& passenger, const PassengerContext& context) const {
    const world::Level& level = context.play.level;
    passenger.floatOnSurface(level.water().row());
    std::optional<int> copter = level.copters().onWaterWithRoom(level.water());
    if (!copter) {
        passenger.changeState(SwimWaving::instance, context);
        return;
    }
    if (passenger.walkTowards(level.copters()[*copter])) Riding::board(passenger, *copter, context);
}

}  // namespace ugh::passengers::route
