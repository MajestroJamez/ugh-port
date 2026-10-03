#include "passengers/route/RoutePassenger.hpp"

#include "passengers/route/NextStop.hpp"

namespace ugh::passengers::route {

RoutePassenger::RoutePassenger(int index, const data::RoutePassengerPlacement& placement)
    : Passenger(index), StateMachine(NextStop::instance), land_(placement.kind), route_(placement.route) {}

void RoutePassenger::update(const PassengerContext& context) { updateState(context); }

void RoutePassenger::frameShown() {
    if (!sprite()) return;
    seenX_ = x().pixels();
    seenY_ = y().pixels();
}

void RoutePassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

const data::AnimatedPassengerKind& RoutePassenger::kind() const {
    if (inWater_) return *land_->swimmer;
    return *land_;
}

void RoutePassenger::standAtDoor(const data::PadDefinition& pad) {
    seenY_ = pad.y - kind().box.y;
    seenX_ = pad.door - kind().box.x;
    moveTo(units::Fixed::fromPixels(seenX_), units::Fixed::fromPixels(seenY_));
}

bool RoutePassenger::walkTowards(const world::Copter& copter) {
    bool step = animate();
    int spot = feetX() - COPTER_DOOR;
    if (spot > copter.pixelX()) {
        show(*kind().walking.left);
        if (step) stepBy(-1);
        return false;
    }
    if (spot < copter.pixelX()) {
        show(*kind().walking.right);
        if (step) stepBy(1);
        return false;
    }
    return true;
}

void RoutePassenger::floatOnSurface(int waterRow) {
    if (waterRow != seenY_) moveToY(units::Fixed::fromPixels(waterRow - kind().box.y));
}

}  // namespace ugh::passengers::route
