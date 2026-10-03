#include "passengers/walking/OnPickupPad.hpp"

#include "passengers/swimming/Splash.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::passengers {

bool OnPickupPad::fellIntoWater(model::Passenger& passenger, model::Level& level) {
    const data::Box& box = passenger.kind().box;
    core::Word surface = level.water().row();
    if ((box.y >> 1) + passenger.pixelY() - surface < 0) return false;
    passenger.moveToY(core::Fixed::fromPixels(surface + box.y));
    level.pad(passenger.pickupPad()).vacate();
    passenger.hideBubble();
    intoWater(passenger, level);
    return true;
}

bool OnPickupPad::knockedIntoWater(model::Passenger& passenger, model::Level& level) {
    int copter = physics::TouchBox(passenger.kind().box, passenger.x(), passenger.y()).firstCopterIn(level);
    if (copter == model::Level::NONE || level.copter(copter).landed()) return false;
    intoWater(passenger, level);
    return true;
}

void OnPickupPad::intoWater(model::Passenger& passenger, model::Level& level) {
    passenger.switchKind();
    passenger.changeState(Splash::instance, level);
}

}  // namespace ugh::passengers
