#include "passengers/route/Splash.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Swimming.hpp"

namespace ugh::passengers::route {

const Splash Splash::instance{};

void Splash::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.route().pickupPad().vacate();
    passenger.plunge();
    context.report({events::EventKind::PassengerInWater, std::nullopt, passenger.index()});
}

/**
 * Down into the water and up again to the surface, where it swims. Quirk of the original: both tests take box.y away
 * from its top instead of adding it (its feet are box.y below its top), so they look at a row above its head: it counts
 * as above the surface longer, and it comes up until that row is at the surface - then it is put with its feet there.
 */
void Splash::update(RoutePassenger& passenger, const PassengerContext& context) const {
    const data::kinds::AnimatedPassengerKind& kind = passenger.kind();
    int surface = context.level.water().row();
    passenger.animate();
    passenger.show(*kind.standing);
    bool aboveSurface = passenger.seenY() - kind.box.y - surface < 0;
    units::Speed speed = aboveSurface ? passenger.swim().fallInAir() : passenger.swim().brakeInWater();
    passenger.moveToY(passenger.y() + speed.perFrame());
    if (speed >= units::Speed()) return;
    if (passenger.y().pixels() - kind.box.y > surface) return;
    passenger.moveToY(units::Fixed::fromPixels(surface - kind.box.y));
    passenger.changeState(Swimming::instance, context);
}

}  // namespace ugh::passengers::route
