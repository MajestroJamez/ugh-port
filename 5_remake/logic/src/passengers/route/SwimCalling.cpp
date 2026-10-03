#include "passengers/route/SwimCalling.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/SwimBoarding.hpp"
#include "passengers/route/SwimWaving.hpp"

namespace ugh::passengers::route {

const SwimCalling SwimCalling::instance{};

/** The same as calling on a pad. */
void SwimCalling::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    Calling::instance.enter(passenger, context);
}

void SwimCalling::update(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.floatOnSurface(context.play.level.water().row());
    if (!context.play.level.copterOnWater()) {
        passenger.changeState(SwimWaving::instance, context);
        return;
    }
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.call().over()) passenger.changeState(SwimBoarding::instance, context);
}

}  // namespace ugh::passengers::route
