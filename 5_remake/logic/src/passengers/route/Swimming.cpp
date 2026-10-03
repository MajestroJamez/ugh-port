#include "passengers/route/Swimming.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Sinking.hpp"
#include "passengers/route/SwimCalling.hpp"

namespace ugh::passengers::route {

const Swimming Swimming::instance{};

void Swimming::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.swim().startAfloat(passenger.kind().swimTime);
}

/** A kind that cannot be rescued only waits to sink. */
void Swimming::update(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.animate();
    passenger.show(*passenger.kind().waving);
    passenger.floatOnSurface(context.play.level.water().row());
    if (passenger.kind().rescuable && context.play.level.copterOnWater(true, true)) {
        passenger.changeState(SwimCalling::instance, context);
        return;
    }
    if (passenger.swim().afloatOver()) passenger.changeState(Sinking::instance, context);
}

}  // namespace ugh::passengers::route
