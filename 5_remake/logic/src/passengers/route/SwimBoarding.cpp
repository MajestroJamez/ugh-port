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
    passenger.floatOnSurface(context.play.level.water().row());
    std::optional<int> copter = context.play.level.copterOnWater(true, false);
    if (!copter) {
        passenger.changeState(SwimWaving::instance, context);
        return;
    }
    if (passenger.walkTowards(context.play.level.copter(*copter))) Riding::board(passenger, *copter, context);
}

}  // namespace ugh::passengers::route
