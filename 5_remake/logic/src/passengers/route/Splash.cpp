#include "passengers/route/Splash.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Swimming.hpp"

namespace ugh::passengers::route {

const Splash Splash::instance{};

void Splash::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    context.play.level.pad(passenger.route().pickupPad()).vacate();
    passenger.hideBubble();
    passenger.restartAnimation();
    passenger.swim().plunge();
    context.play.report({events::EventKind::PassengerInWater, std::nullopt, passenger.index()});
}

void Splash::update(RoutePassenger& passenger, const PassengerContext& context) const {
    const data::PassengerKind& kind = passenger.kind();
    units::Int16 surface = context.play.level.water().row();
    passenger.animate();
    passenger.show(*kind.standing);
    units::Speed speed = passenger.swim().splash(passenger.seenY() - kind.box.y - surface < 0);
    passenger.moveToY(passenger.y() + speed.perFrame());
    if (speed >= units::Speed()) return;
    if (passenger.y().pixels() - kind.box.y > surface) return;
    passenger.moveToY(units::Fixed::fromPixels(surface - kind.box.y));
    passenger.changeState(Swimming::instance, context);
}

}  // namespace ugh::passengers::route
