#include "passengers/route/Swimming.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Sinking.hpp"
#include "passengers/route/SwimCalling.hpp"

namespace ugh::passengers::route {

const Swimming Swimming::instance{};

void Swimming::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.swim().startAfloat(passenger.form().swimmer().swimTime);
}

/** A kind that cannot be rescued only waits to sink. */
void Swimming::update(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Level& level = context.level;
    passenger.animate();
    passenger.show(*passenger.kind().waving);
    passenger.floatOnSurface(level.water().row());
    if (passenger.form().swimmer().rescuable && level.copters().stillOnWaterWithRoom(level.water())) {
        passenger.changeState(SwimCalling::instance, context);
        return;
    }
    if (passenger.swim().afloatOver()) passenger.changeState(Sinking::instance, context);
}

}  // namespace ugh::passengers::route
