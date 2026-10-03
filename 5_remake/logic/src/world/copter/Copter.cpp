#include "world/copter/Copter.hpp"

namespace ugh::world::copter {

void Copter::placeAtStart(units::Fixed x, units::Fixed y, int firstRotorSprite) {
    motion_.moveToX(x);
    motion_.moveToY(y);
    motion_.stop();
    landedOn_ = nullptr;
    rotor_.start(firstRotorSprite);
    cabin_.clear();
}

void Copter::land(const scenery::Pad& pad) {
    landedOn_ = &pad;
    motion_.stop();
}

void Copter::throwUp(units::Fixed walkerSpeed) {
    motion_.throwUp(walkerSpeed);
    landedOn_ = nullptr;
}

}  // namespace ugh::world::copter
