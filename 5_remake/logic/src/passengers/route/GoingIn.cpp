#include "passengers/route/GoingIn.hpp"

#include "passengers/route/NextStop.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const GoingIn GoingIn::instance{};

void GoingIn::enter(RoutePassenger& passenger, const PassengerContext&) const { passenger.restartAnimation(); }

void GoingIn::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (!passenger.animate()) return;
    const data::kinds::Animation& door = *passenger.kinds().land().goingIn;
    passenger.showFrameOf(door);
    if (!passenger.atLastFrameOf(door)) return;
    passenger.route().next();
    passenger.continueIn(NextStop::instance, context);
}

}  // namespace ugh::passengers::route
