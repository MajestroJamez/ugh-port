#include "passengers/route/SwimCalling.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/SwimBoarding.hpp"
#include "passengers/route/SwimWaving.hpp"

namespace ugh::passengers::route {

const SwimCalling SwimCalling::instance{};

/** The same as calling on a pad. */
void SwimCalling::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.startCalling(context.data.sprites());
}

void SwimCalling::update(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Level& level = context.level;
    passenger.floatOnSurface(level.water().row());
    if (!level.copters().onWater(level.water())) {
        passenger.changeState(SwimWaving::instance, context);
        return;
    }
    passenger.wave();
    if (passenger.pickupWait().tick()) passenger.changeState(SwimBoarding::instance, context);
}

}  // namespace ugh::passengers::route
