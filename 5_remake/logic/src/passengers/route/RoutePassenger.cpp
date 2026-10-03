#include "passengers/route/RoutePassenger.hpp"

#include "passengers/route/NextStop.hpp"

namespace ugh::passengers::route {

RoutePassenger::RoutePassenger(int index, const data::RoutePassengerPlacement& placement)
    : Passenger(index), StateMachine(NextStop::instance), kind_(&placement.kind()), route_(placement.route()) {}

void RoutePassenger::update(const PassengerContext& context) { updateState(context); }

void RoutePassenger::frameShown() {
    if (!sprite()) return;
    seenX_ = x().pixels();
    seenY_ = y().pixels();
}

void RoutePassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

void RoutePassenger::standAtDoor(const data::PadDefinition& pad) {
    seenY_ = pad.y - kind_->box.y;
    seenX_ = pad.door - kind_->box.x;
    moveTo(units::Fixed::fromPixels(seenX_), units::Fixed::fromPixels(seenY_));
}

bool RoutePassenger::walkTowards(const world::Copter& copter) {
    bool step = animate();
    units::Int16 spot = feetX() - COPTER_DOOR;
    if (spot > copter.pixelX()) {
        show(*kind_->walking.left);
        if (step) stepBy(-1);
        return false;
    }
    if (spot < copter.pixelX()) {
        show(*kind_->walking.right);
        if (step) stepBy(1);
        return false;
    }
    return true;
}

void RoutePassenger::floatOnSurface(units::Int16 waterRow) {
    if (waterRow != seenY_) moveToY(units::Fixed::fromPixels(waterRow - kind_->box.y));
}

}  // namespace ugh::passengers::route
