#include "enemies/flyer/Falling.hpp"

#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Placed.hpp"
#include "world/scenery/Screen.hpp"

namespace ugh::enemies::flyer {

namespace {

using world::scenery::Screen;

constexpr units::Fixed MAX_FALL_SPEED = units::Fixed::fromRaw(40);   // per frame

}  // namespace

const Falling Falling::instance{};

void Falling::enter(Flyer& flyer, const EnemyContext& context) const {
    flyer.scoreStun(flyer.kind().score, context);
    flyer.startFalling();
    flyer.showSprite(flyer.speedX() < units::Fixed() ? flyer.kind().hitSpriteLeft : flyer.kind().hitSpriteRight);
}

void Falling::update(Flyer& flyer, const EnemyContext& context) const {
    units::Fixed x = flyer.speedX() + flyer.x();
    if (Screen::pastSide(x, Screen::FLYER_LEFT)) {
        flyer.continueIn(Placed::instance, context);
        return;
    }
    flyer.moveToX(x);
    flyer.fallFaster(MAX_FALL_SPEED);
    units::Fixed y = flyer.fallSpeed() + flyer.y();
    if (y >= Screen::BOTTOM) {
        flyer.continueIn(Placed::instance, context);
        return;
    }
    flyer.moveToY(y);
}

}  // namespace ugh::enemies::flyer
