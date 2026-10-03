#include "model/Copter.hpp"

#include <algorithm>

namespace ugh::model {

namespace {

constexpr core::Word HANGING_TARGET = 7;     // the pad number shown while the standing passenger hangs below
constexpr core::Word ROTOR_MAX_EFFORT = 0x5a;   // the rotor spins no faster than with this effort
constexpr core::Word ROTOR_PERIOD = 0xc8;       // effort per rotor sprite

}  // namespace

void Copter::placeAtStart(core::Fixed x, core::Fixed y, data::Sprite rotor) {
    moveToX(x);
    moveToY(y);
    s_.landedPad = IN_THE_AIR;
    s_.rotorCounter = -1;
    s_.rotor = rotor;
    s_.carrying = 0;
    s_.targetPad = 0;
    s_.fare = 0;
    s_.vx = core::Speed(0);
    s_.vy = core::Speed(0);
}

void Copter::startFrame() {
    s_.impact = 0;
    s_.effort = 0;
}

void Copter::setSpeed(core::Speed vx, core::Speed vy) {
    s_.vx = vx;
    s_.vy = vy;
}

void Copter::moveToX(core::Fixed x) {
    s_.x = x;
    s_.pixelX = x.pixels();
}

void Copter::moveToY(core::Fixed y) {
    s_.y = y;
    s_.pixelY = y.pixels();
}

void Copter::land(int pad) {
    s_.landedPad = pad;
    s_.vx = core::Speed(0);
    s_.vy = core::Speed(0);
}

void Copter::throwUp(core::Word speed) {
    s_.y -= core::Fixed::fromPixels(1);   // the pixel position stays: the physics catches up next frame
    s_.vx = core::Speed(speed << 5);   // the original shifts the walker's speed: 5 bits sideways, 4 bits up
    s_.vy = core::Speed(speed << 4);
    s_.landedPad = IN_THE_AIR;
}

void Copter::spinRotor(data::Sprite first, data::Sprite end) {
    s_.rotorCounter -= std::min(s_.effort, ROTOR_MAX_EFFORT);
    if (s_.rotorCounter >= 0) return;
    s_.rotorCounter += ROTOR_PERIOD;
    core::Word next = s_.rotor + 1;
    if (next >= core::Word(end)) next = first;
    s_.rotor = next.bits();
}

void Copter::takeOnBoard(core::Word look, core::Word targetNumber, core::Word fare, core::Word fareMin) {
    s_.carrying = look;
    s_.targetPad = targetNumber;
    s_.fare = fare;
    s_.fareMin = fareMin;
}

void Copter::pickUpHanging(core::Word look) {
    s_.carrying = look;
    s_.targetPad = HANGING_TARGET;
}

void Copter::lowerFare() {
    if (core::Word::unsignedLess(s_.fareMin, s_.fare)) --s_.fare;
}

void Copter::unload() {
    s_.carrying = 0;
    s_.targetPad = 0;
}

}  // namespace ugh::model
