#include "passengers/route/RoutePassenger.hpp"

#include "passengers/route/NextStop.hpp"
#include "world/copter/CopterShape.hpp"

namespace ugh::passengers::route {

RoutePassenger::RoutePassenger(int index, const data::levels::RoutePassengerPlacement& placement, world::Level& level)
    : Passenger(index), StateMachine(NextStop::instance), form_(*placement.kind), route_(placement.route, level) {}

void RoutePassenger::update(const PassengerContext& context) { updateState(context); }

void RoutePassenger::frameShown() {
    if (!sprite()) return;
    seenX_ = x().pixels();
    seenY_ = y().pixels();
}

void RoutePassenger::accept(PassengerVisitor& visitor) const { visitor.visit(*this); }

void RoutePassenger::startCalling(const data::SpriteIds& sprites) {
    restartAnimation();
    int bubble = sprites.firstDestinationBubble + route_.targetPad().index();
    if (bubble > sprites.lastDestinationBubble) bubble = sprites.lastDestinationBubble;
    showBubble(bubble);
    pickupWait_.startCalling();
}

void RoutePassenger::startWaving(const data::SpriteIds& sprites) {
    restartAnimation();
    showBubble(sprites.impatientBubble);
    pickupWait_.startWaving();
}

void RoutePassenger::startBoarding() {
    restartAnimation();
    hideBubble();
}

void RoutePassenger::standAtDoor(const data::levels::PadDefinition& pad) {
    seenY_ = pad.y - kind().box.y;
    seenX_ = pad.door - kind().box.x;
    moveTo(units::Fixed::fromPixels(seenX_), units::Fixed::fromPixels(seenY_));
}

bool RoutePassenger::walkTowards(const world::copter::Copter& copter) {
    bool stepDue = animate();
    int door = copter.motion().pixelX() + world::copter::CopterShape::DOOR_X;
    return stepDue ? stepTowards(door) : faceTowards(door);
}

bool RoutePassenger::stepTowards(int feet) {
    if (faceTowards(feet)) return true;
    moveToX(x() + units::Fixed::fromPixels(feetX() < feet ? 1 : -1));
    return false;
}

bool RoutePassenger::faceTowards(int feet) {
    if (feetX() == feet) return true;
    show(kind().walking.towards(feetX() < feet ? data::kinds::Facing::Right : data::kinds::Facing::Left));
    return false;
}

void RoutePassenger::floatOnSurface(int waterRow) {
    if (waterRow != seenY_) moveToY(units::Fixed::fromPixels(waterRow - kind().box.y));
}

}  // namespace ugh::passengers::route
