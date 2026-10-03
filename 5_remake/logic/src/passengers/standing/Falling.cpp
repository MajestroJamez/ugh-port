#include "passengers/standing/Falling.hpp"

#include "passengers/standing/Gone.hpp"
#include "passengers/standing/Placed.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "physics/Ballistics.hpp"

namespace ugh::passengers::standing {

namespace {

using units::Fixed;

constexpr units::Int16 GRAVITY = 2;   // 1/32 px per frame, per frame
// where it falls from: from the copter's top left corner, in pixels
constexpr units::Int16 DROP_X = 16, DROP_Y = 10;

}  // namespace

const Falling Falling::instance{};

/** It falls from under the copter; the copter's speed in 1/64 Fixed becomes twice that in 1/32 px (shifted by 5). */
void Falling::enter(StandingPassenger& passenger, const PassengerContext& context) const {
    int player = *passenger.carrier();
    world::Copter& copter = context.play.level.copter(player);
    const data::Box& box = passenger.kind().box;
    copter.unload();
    passenger.setFall(Fixed::fromRaw(copter.speedX().raw() >> 5), copter.speedY().raw() >> 5);
    passenger.moveTo(copter.x() + Fixed::fromPixels(DROP_X) - Fixed::fromPixels(box.x),
                     copter.y() + Fixed::fromPixels(DROP_Y) - Fixed::fromRaw(box.y << 4));   // half its height
    passenger.showSprite(context.play.data.sprites().droppedPassenger);
    context.play.report({events::EventKind::PassengerDropped, player, passenger.index()});
}

void Falling::update(StandingPassenger& passenger, const PassengerContext& context) const {
    const data::Box& box = passenger.kind().box;
    physics::Ballistics::Body body{passenger.x(), passenger.y(), box.x, box.y, passenger.dropSpeedX(), passenger.fallSpeed()};
    physics::Ballistics::Result result =
        physics::Ballistics(GRAVITY, physics::Ballistics::Landing::Passenger).fall(body, context.play.level);
    if (result == physics::Ballistics::Result::Gone) {
        passenger.changeState(Gone::instance, context);
        return;
    }
    passenger.moveTo(body.x, body.y);
    passenger.setFall(body.speedX, body.fallSpeed);
    if (result == physics::Ballistics::Result::Landed) passenger.continueIn(Placed::instance, context);
}

}  // namespace ugh::passengers::standing
