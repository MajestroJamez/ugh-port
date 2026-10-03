#include "passengers/route/RoutePassenger.hpp"

#include "passengers/route/NextStop.hpp"

namespace ugh::passengers::route {

namespace {

constexpr units::Int16 COPTER_DOOR = 16;   // from the copter's left edge to where a passenger gets in

}  // namespace

RoutePassenger::RoutePassenger(int index, const data::RoutePassengerPlacement& placement)
    : Passenger(index), kind_(&placement.kind()), route_(&placement.route()), state_(&NextStop::instance) {}

void RoutePassenger::update(const PassengerContext& context) { state_->update(*this, context); }

void RoutePassenger::frameShown() {
    if (!sprite_) return;
    seenX_ = x_.pixels();
    seenY_ = y_.pixels();
}

void RoutePassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

void RoutePassenger::changeState(const RouteState& next, const PassengerContext& context) {
    state_ = &next;
    next.enter(*this, context);
}

void RoutePassenger::continueIn(const RouteState& next, const PassengerContext& context) {
    changeState(next, context);
    next.update(*this, context);
}

bool RoutePassenger::arrivalDue() {
    if (arrivalDelay_ == 0) return true;
    arrivalDelay_ -= 1;
    return arrivalDelay_ == 0;
}

void RoutePassenger::standAtDoor(const data::PadDefinition& pad) {
    seenY_ = pad.y - kind_->box.y;
    y_ = units::Fixed::fromPixels(seenY_);
    seenX_ = pad.door - kind_->box.x;
    x_ = units::Fixed::fromPixels(seenX_);
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
    if (waterRow != seenY_) y_ = units::Fixed::fromPixels(waterRow - kind_->box.y);
}

}  // namespace ugh::passengers::route
