#include "passengers/route/SwimWaving.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Sinking.hpp"

namespace ugh::passengers::route {

const SwimWaving SwimWaving::instance{};

/** The same as waving impatiently on a pad. */
void SwimWaving::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.startWaving(context.data.sprites());
}

void SwimWaving::update(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.floatOnSurface(context.level.water().row());
    passenger.wave();
    if (passenger.pickupWait().over()) passenger.changeState(Sinking::instance, context);
}

}  // namespace ugh::passengers::route
