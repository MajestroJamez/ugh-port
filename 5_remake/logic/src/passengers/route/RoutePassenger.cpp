#include "passengers/route/RoutePassenger.hpp"

#include "passengers/route/NextStop.hpp"

namespace ugh::passengers::route {

RoutePassenger::RoutePassenger(int index, const data::levels::RoutePassengerPlacement& placement)
    : Passenger(index), StateMachine(NextStop::instance), kinds_(*placement.kind), route_(placement.route) {}

void RoutePassenger::update(const PassengerContext& context) { updateState(context); }

void RoutePassenger::frameShown() {
    if (!sprite()) return;
    seenX_ = x().pixels();
    seenY_ = y().pixels();
}

void RoutePassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

void RoutePassenger::standAtDoor(const data::levels::PadDefinition& pad) {
    seenY_ = pad.y - kind().box.y;
    seenX_ = pad.door - kind().box.x;
    moveTo(units::Fixed::fromPixels(seenX_), units::Fixed::fromPixels(seenY_));
}

bool RoutePassenger::walkTowards(const world::Copter& copter) {
    bool stepDue = animate();
    return stepTowards(copter.pixelX() + COPTER_DOOR, stepDue);
}

bool RoutePassenger::stepTowards(int feet, bool stepDue) {
    if (feetX() == feet) return true;
    bool right = feetX() < feet;
    show(kind().walking.towards(right));
    if (stepDue) moveToX(x() + units::Fixed::fromPixels(right ? 1 : -1));
    return false;
}

void RoutePassenger::floatOnSurface(int waterRow) {
    if (waterRow != seenY_) moveToY(units::Fixed::fromPixels(waterRow - kind().box.y));
}

}  // namespace ugh::passengers::route
