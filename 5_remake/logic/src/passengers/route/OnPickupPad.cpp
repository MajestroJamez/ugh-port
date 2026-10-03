#include "passengers/route/OnPickupPad.hpp"

#include "passengers/route/Splash.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::passengers::route {

void OnPickupPad::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (fellIntoWater(passenger, context)) return;
    walk(passenger, context);
    if (knockedIntoWater(passenger, context)) return;
    stay(passenger, context);
}

/** The water rose to its knees: it is in the water. */
bool OnPickupPad::fellIntoWater(RoutePassenger& passenger, const PassengerContext& context) {
    const data::kinds::Box& box = passenger.kind().box;
    int surface = context.level.water().row();
    if ((box.y >> 1) + passenger.seenY() - surface < 0) return false;
    passenger.moveToY(units::Fixed::fromPixels(surface + box.y));
    passenger.route().pickupPad().vacate();
    passenger.hideBubble();
    intoWater(passenger, context);
    return true;
}

/** A copter in the air touched it: it is knocked into the water. */
bool OnPickupPad::knockedIntoWater(RoutePassenger& passenger, const PassengerContext& context) {
    const world::Copter* copter =
        physics::TouchBox(passenger.kind().box, passenger.x(), passenger.y()).firstCopterIn(context.level.copters());
    if (!copter || copter->landed()) return false;
    intoWater(passenger, context);
    return true;
}

void OnPickupPad::intoWater(RoutePassenger& passenger, const PassengerContext& context) {
    passenger.kinds().intoWater();
    passenger.changeState(Splash::instance, context);
}

}  // namespace ugh::passengers::route
