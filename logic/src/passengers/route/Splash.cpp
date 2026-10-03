#include "passengers/route/Splash.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Swimming.hpp"

namespace ugh::passengers::route {

namespace {

using units::Speed;

// in 1/64 Fixed per frame, per frame
constexpr Speed WATER_BRAKE = Speed::fromRaw(373);    // going down in the water
constexpr Speed BUOYANCY = Speed::fromRaw(92);        // coming up
constexpr Speed MAX_SPEED = Speed::fromRaw(6144);

}  // namespace

const Splash Splash::instance{};

void Splash::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    context.play.level.pad(passenger.pickupPad()).vacate();
    passenger.hideBubble();
    passenger.restartAnimation();
    passenger.setSwimSpeed(Speed());
    context.play.report({events::EventKind::PassengerInWater, std::nullopt, passenger.index()});
}

void Splash::update(RoutePassenger& passenger, const PassengerContext& context) const {
    const data::PassengerKind& kind = passenger.kind();
    units::Int16 surface = context.play.level.water().row();
    passenger.animate();
    passenger.show(*kind.standing);
    Speed speed = passenger.swimSpeed();
    if (passenger.seenY() - kind.box.y - surface < 0) speed += GRAVITY;
    else if (speed > Speed()) speed -= WATER_BRAKE;
    else speed -= BUOYANCY;
    speed = speed.clamped(MAX_SPEED);
    passenger.setSwimSpeed(speed);
    passenger.moveToY(passenger.y() + speed.perFrame());
    if (speed >= Speed()) return;
    if (passenger.y().pixels() - kind.box.y > surface) return;
    passenger.moveToY(units::Fixed::fromPixels(surface - kind.box.y));
    passenger.changeState(Swimming::instance, context);
}

}  // namespace ugh::passengers::route
