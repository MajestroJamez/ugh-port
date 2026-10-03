#include "world/Copter.hpp"

namespace ugh::world {

namespace {

constexpr int ROTOR_MAX_EFFORT = 90;   // the rotor spins no faster than with this effort
constexpr int ROTOR_PERIOD = 200;      // effort per rotor sprite

}  // namespace

void Copter::placeAtStart(units::Fixed x, units::Fixed y, int firstRotorSprite) {
    moveToX(x);
    moveToY(y);
    landedPad_.reset();
    rotorCounter_ = -1;
    rotorSprite_ = firstRotorSprite;
    cargo_.reset();
    fare_ = 0;
    vx_ = units::Speed();
    vy_ = units::Speed();
}

void Copter::setSpeed(units::Speed vx, units::Speed vy) {
    vx_ = vx;
    vy_ = vy;
}

void Copter::moveToX(units::Fixed x) {
    x_ = x;
    pixelX_ = x.pixels();
}

void Copter::moveToY(units::Fixed y) {
    y_ = y;
    pixelY_ = y.pixels();
}

void Copter::land(int pad) {
    landedPad_ = pad;
    vx_ = units::Speed();
    vy_ = units::Speed();
}

void Copter::throwUp(int walkerSpeed) {
    y_ -= units::Fixed::fromPixels(1);
    vx_ = units::Speed::fromRaw(walkerSpeed << 5);
    vy_ = units::Speed::fromRaw(walkerSpeed << 4);
    landedPad_.reset();
}

void Copter::spinRotor(int firstSprite, int lastSprite) {
    rotorCounter_ -= effort_ < ROTOR_MAX_EFFORT ? effort_ : ROTOR_MAX_EFFORT;
    if (rotorCounter_ >= 0) return;
    rotorCounter_ += ROTOR_PERIOD;
    rotorSprite_ = rotorSprite_ + 1 > lastSprite ? firstSprite : rotorSprite_ + 1;
}

void Copter::takeOnBoard(int look, int destination, int fare, int fareMin) {
    cargo_ = Cargo{look, destination, fareMin};
    fare_ = fare;
}

void Copter::pickUpHanging(int look) { cargo_ = Cargo{look, std::nullopt, 0}; }

void Copter::lowerFare() {
    if (cargo_ && fare_ > cargo_->fareMin) fare_ -= 1;
}

void Copter::placeByTestPilot(units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx,
                              units::Speed vy, std::optional<int> landedPad) {
    x_ = x;
    y_ = y;
    pixelX_ = pixelX;
    pixelY_ = pixelY;
    vx_ = vx;
    vy_ = vy;
    landedPad_ = landedPad;
}

}  // namespace ugh::world
