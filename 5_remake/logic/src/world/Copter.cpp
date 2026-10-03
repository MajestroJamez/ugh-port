#include "world/Copter.hpp"

namespace ugh::world {

void Copter::placeAtStart(units::Fixed x, units::Fixed y, int firstRotorSprite) {
    moveToX(x);
    moveToY(y);
    landedPad_.reset();
    rotor_.start(firstRotorSprite);
    cabin_.clear();
    setSpeed(units::Speed(), units::Speed());
}

void Copter::land(const Pad& pad) {
    landedPad_ = pad.index();
    setSpeed(units::Speed(), units::Speed());
}

void Copter::throwUp(int walkerSpeed) {
    thrown(walkerSpeed);
    landedPad_.reset();
}

}  // namespace ugh::world
