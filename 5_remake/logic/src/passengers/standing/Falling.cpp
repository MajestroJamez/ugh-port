#include "passengers/standing/Falling.hpp"

#include "passengers/standing/Gone.hpp"
#include "passengers/standing/Placed.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "physics/Ballistics.hpp"

namespace ugh::passengers::standing {

namespace {

using units::Fixed;

constexpr Fixed GRAVITY = Fixed::fromRaw(2);   // per frame, per frame

}  // namespace

const Falling Falling::instance{};

/** It falls from under the copter (`world::copter::CopterShape::DROP`) with twice the copter's speed. */
void Falling::enter(StandingPassenger& passenger, const PassengerContext& context) const {
    world::copter::Copter& copter = *passenger.carrier();
    const data::kinds::Box& box = passenger.kind().box;
    copter.cabin().unload();
    passenger.setFall(copter.motion().speedX().twicePerFrame(), copter.motion().speedY().twicePerFrame());
    passenger.moveTo(copter.dropX() - Fixed::fromPixels(box.x),
                     copter.dropY() - Fixed::fromPixels(box.y).half());   // half its height
    passenger.showSprite(context.data.sprites().droppedPassenger);
    context.report({events::EventKind::PassengerDropped, copter.player(), passenger.index()});
}

void Falling::update(StandingPassenger& passenger, const PassengerContext& context) const {
    const data::kinds::Box& box = passenger.kind().box;
    physics::Ballistics::Body body{passenger.x(), passenger.y(), box.x, box.y, passenger.dropSpeedX(), passenger.fallSpeed()};
    physics::Ballistics::Result result =
        physics::Ballistics(GRAVITY, physics::Ballistics::Landing::Passenger).fall(body, context.level);
    if (result == physics::Ballistics::Result::Gone) {
        passenger.changeState(Gone::instance, context);
        return;
    }
    passenger.moveTo(body.x, body.y);
    passenger.setFall(body.speedX, body.fallSpeed);
    if (result == physics::Ballistics::Result::Landed) passenger.continueIn(Placed::instance, context);
}

}  // namespace ugh::passengers::standing
