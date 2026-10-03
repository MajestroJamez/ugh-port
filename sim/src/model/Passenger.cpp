#include "model/Passenger.hpp"

#include <string>

#include "core/AmigaScale.hpp"
#include "core/Audit.hpp"
#include "model/Copter.hpp"
#include "passengers/PassengerState.hpp"

namespace ugh::model {

namespace {

constexpr core::Word QUICK_DELIVERY_TIME = 0xc8;   // frames of riding that still earn a bonus item
constexpr core::Word COPTER_DOOR = 0x10;           // from the copter's left edge to where a passenger gets in

}  // namespace

void Passenger::placeWalking(const data::PassengerPlacement& placement, const passengers::PassengerState& state) {
    s_.kind = placement.kind;
    s_.startPad = placement.pad;
    s_.route = placement.route;
    s_.sprite = data::NO_SPRITE;
    s_.bubble = data::NO_SPRITE;
    s_.state = &state;
}

void Passenger::placeStanding(const data::PassengerPlacement& placement, const passengers::PassengerState& state) {
    placeWalking(placement, state);
    s_.x = placement.x;
    s_.y = placement.y;
}

void Passenger::update(Level& level) {
    core::audit::Scope scope(core::audit::intern(std::string("passenger ") + s_.state->name()));
    s_.state->update(*this, level);
}

void Passenger::changeState(const passengers::PassengerState& next, Level& level) {
    s_.state = &next;
    next.enter(*this, level);
}

void Passenger::continueIn(const passengers::PassengerState& next, Level& level) {
    changeState(next, level);
    next.update(*this, level);
}

void Passenger::startStop() {
    s_.targetPad = s_.route.to();
    s_.pickupPad = s_.route.from();
    s_.timer.startArrivalDelay(core::amigaFramesToPc(s_.route.delay()));
}

void Passenger::moveTo(core::Fixed x, core::Fixed y) {
    s_.x = x;
    s_.y = y;
}

void Passenger::standAtDoor(core::Word doorX, core::Word padY) {
    s_.pixelY = padY - s_.kind->box.y;
    s_.y = core::Fixed::fromPixels(s_.pixelY);
    s_.pixelX = doorX - s_.kind->box.x;
    s_.x = core::Fixed::fromPixels(s_.pixelX);
}

bool Passenger::walkTowards(const Copter& copter) {
    bool step = animate();
    core::Word spot = feetX() - COPTER_DOOR;
    if (spot > copter.pixelX()) {
        show(*s_.kind->walking.left);
        if (step) stepBy(-1);
        return false;
    }
    if (spot < copter.pixelX()) {
        show(*s_.kind->walking.right);
        if (step) stepBy(1);
        return false;
    }
    return true;
}

void Passenger::floatOnSurface(core::Word waterRow) {
    if (waterRow != s_.pixelY) s_.y = core::Fixed::fromPixels(waterRow - s_.kind->box.y);
}

void Passenger::updatePixels() {
    if (hidden()) return;
    s_.pixelX = s_.x.pixels();
    s_.pixelY = s_.y.pixels();
}

bool Passenger::fallingDown() const {
    if (s_.kind->type == data::PassengerKind::Type::Standing && s_.state->fallsDown()) vyReading("fall");
    return s_.kind->type == data::PassengerKind::Type::Standing && s_.state->fallsDown() && s_.vy >= 0;
}

void Passenger::startQuickDeliveryTime() { s_.bonusTimer = QUICK_DELIVERY_TIME; s_.bonusTimerSet = true; }

void Passenger::tickQuickDeliveryTime() {
    if (!s_.bonusTimerSet) core::audit::count("Q2 Passenger.bonusTimer read unset");
    if (s_.bonusTimer > 0) --s_.bonusTimer;
}

void Passenger::vyReading(const char* meaning) const {
    if (std::string(s_.vyMeaning) != meaning)
        core::audit::count(std::string("Q2 Passenger.vy read-as=") + meaning + " written-as=" + s_.vyMeaning);
    else
        core::audit::count(std::string("Q2ok Passenger.vy ") + meaning);
}

}  // namespace ugh::model
