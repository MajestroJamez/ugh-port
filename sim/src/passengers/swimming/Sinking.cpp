#include "passengers/swimming/Sinking.hpp"

#include "passengers/Gone.hpp"

namespace ugh::passengers {

namespace {

constexpr core::Speed GRAVITY(0x27);         // 1/64 Fixed per frame, per frame
constexpr core::Word BOTTOM_EDGE = 0x1800;   // gone below this (compared unsigned)

}  // namespace

const Sinking Sinking::instance{};

/** 113b:1e9c */
void Sinking::enter(model::Passenger& passenger, model::Level&) const {
    passenger.hideBubble();
    passenger.restartAnimation();
    passenger.setSwimSpeed(core::Speed(0));
}

/** 113b:1ec0 - drowns: sinks until it is off the bottom. */
void Sinking::update(model::Passenger& passenger, model::Level& level) const {
    passenger.animate();
    passenger.show(*passenger.kind().standing);
    core::Speed speed = passenger.swimSpeed() + GRAVITY;
    passenger.setSwimSpeed(speed);
    core::Fixed y = passenger.y() + speed.perFrame();
    if (core::Word::unsignedLess(y.raw(), BOTTOM_EDGE)) {
        passenger.moveToY(y);
        return;
    }
    passenger.changeState(Gone::instance, level);
}

}  // namespace ugh::passengers
