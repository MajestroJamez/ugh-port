#include "passengers/swimming/Splash.hpp"

#include "passengers/swimming/Swimming.hpp"

namespace ugh::passengers {

namespace {

using core::Speed;

// in 1/64 Fixed per frame, per frame
constexpr Speed GRAVITY(0x27);         // above the water
constexpr Speed WATER_BRAKE(0x175);    // going down in the water
constexpr Speed BUOYANCY(0x5c);        // coming up
constexpr Speed MAX_SPEED(0x1800);

}  // namespace

const Splash Splash::instance{};

/** 113b:1da8 - into the water: the pickup pad is free again. */
void Splash::enter(model::Passenger& passenger, model::Level& level) const {
    level.pad(passenger.pickupPad()).vacate();
    passenger.hideBubble();
    passenger.restartAnimation();
    passenger.setSwimSpeed(Speed(0));
    level.report({core::EventKind::PassengerInWater, core::Event::NONE, passenger.index()});
}

/** 113b:1dd5 - falls into the water, goes under and floats up to the surface. */
void Splash::update(model::Passenger& passenger, model::Level& level) const {
    const data::PassengerKind& kind = passenger.kind();
    core::Word surface = level.water().row();
    passenger.animate();
    passenger.show(*kind.standing);
    Speed speed = passenger.swimSpeed();
    if (passenger.pixelY() - kind.box.y - surface < 0) speed += GRAVITY;
    else if (speed > Speed(0)) speed -= WATER_BRAKE;
    else speed -= BUOYANCY;
    speed = speed.clamped(MAX_SPEED);
    passenger.setSwimSpeed(speed);
    passenger.moveToY(passenger.y() + speed.perFrame());
    if (speed >= Speed(0)) return;
    if (passenger.y().pixels() - kind.box.y > surface) return;
    passenger.moveToY(core::Fixed::fromPixels(surface - kind.box.y));
    passenger.changeState(Swimming::instance, level);
}

}  // namespace ugh::passengers
