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

void Copter::land(int pad) {
    landedPad_ = pad;
    setSpeed(units::Speed(), units::Speed());
}

void Copter::throwUp(int walkerSpeed) {
    thrown(walkerSpeed);
    landedPad_.reset();
}

void Copter::placeByTestPilot(units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx, units::Speed vy,
                              std::optional<int> landedPad) {
    place(x, y, pixelX, pixelY, vx, vy);
    landedPad_ = landedPad;
}

}  // namespace ugh::world
