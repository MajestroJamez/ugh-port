#include "passengers/route/Sinking.hpp"

#include "passengers/route/Gone.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "world/Screen.hpp"

namespace ugh::passengers::route {

namespace {

constexpr units::Speed GRAVITY = units::Speed::fromRaw(39);   // 1/64 Fixed per frame, per frame

}  // namespace

const Sinking Sinking::instance{};

void Sinking::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.hideBubble();
    passenger.restartAnimation();
    passenger.setSwimSpeed(units::Speed());
}

void Sinking::update(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.animate();
    passenger.show(*passenger.kind().standing);
    units::Speed speed = passenger.swimSpeed() + GRAVITY;
    passenger.setSwimSpeed(speed);
    units::Fixed y = passenger.y() + speed.perFrame();
    // compared unsigned, as the original does
    if (units::Int16::unsignedLess(y.raw(), world::Screen::BOTTOM.raw())) {
        passenger.moveToY(y);
        return;
    }
    passenger.changeState(Gone::instance, context);
}

}  // namespace ugh::passengers::route
