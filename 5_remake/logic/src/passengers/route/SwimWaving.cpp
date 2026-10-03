#include "passengers/route/SwimWaving.hpp"

#include "passengers/route/Impatient.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Sinking.hpp"

namespace ugh::passengers::route {

const SwimWaving SwimWaving::instance{};

/** The same as waving impatiently on a pad. */
void SwimWaving::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    Impatient::instance.enter(passenger, context);
}

void SwimWaving::update(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.floatOnSurface(context.level.water().row());
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.call().over()) passenger.changeState(Sinking::instance, context);
}

}  // namespace ugh::passengers::route
