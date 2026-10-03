#include "passengers/route/OnPickupPad.hpp"

#include "passengers/route/Splash.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::passengers::route {

bool OnPickupPad::fellIntoWater(RoutePassenger& passenger, const PassengerContext& context) {
    const data::Box& box = passenger.kind().box;
    units::Int16 surface = context.play.level.water().row();
    if ((box.y >> 1) + passenger.seenY() - surface < 0) return false;
    passenger.moveToY(units::Fixed::fromPixels(surface + box.y));
    context.play.level.pad(passenger.route().pickupPad()).vacate();
    passenger.hideBubble();
    intoWater(passenger, context);
    return true;
}

bool OnPickupPad::knockedIntoWater(RoutePassenger& passenger, const PassengerContext& context) {
    std::optional<int> copter =
        physics::TouchBox(passenger.kind().box, passenger.x(), passenger.y()).firstCopterIn(context.play.level);
    if (!copter || context.play.level.copter(*copter).landed()) return false;
    intoWater(passenger, context);
    return true;
}

void OnPickupPad::intoWater(RoutePassenger& passenger, const PassengerContext& context) {
    passenger.intoWater();
    passenger.changeState(Splash::instance, context);
}

}  // namespace ugh::passengers::route
