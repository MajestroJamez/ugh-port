#include "enemies/flyer/Falling.hpp"

#include "enemies/flyer/Flyer.hpp"
#include "enemies/flyer/Placed.hpp"
#include "world/Screen.hpp"

namespace ugh::enemies::flyer {

namespace {

constexpr int MAX_FALL_SPEED = 40;   // 1/32 px per frame

}  // namespace

const Falling Falling::instance{};

void Falling::enter(Flyer& flyer, const EnemyContext& context) const {
    flyer.scoreStun(flyer.kind().score, context);
    flyer.startFalling();
    flyer.showSprite(flyer.speedX() < units::Fixed() ? flyer.kind().hitSpriteLeft : flyer.kind().hitSpriteRight);
}

void Falling::update(Flyer& flyer, const EnemyContext& context) const {
    units::Fixed x = flyer.speedX() + flyer.x();
    if (world::Screen::pastSide(x, world::Screen::FLYER_LEFT)) {
        flyer.continueIn(Placed::instance, context);
        return;
    }
    flyer.moveToX(x);
    flyer.fallFaster(MAX_FALL_SPEED);
    units::Fixed y = units::Fixed::fromRaw(flyer.fallSpeed()) + flyer.y();
    if (y >= world::Screen::BOTTOM) {
        flyer.continueIn(Placed::instance, context);
        return;
    }
    flyer.moveToY(y);
}

}  // namespace ugh::enemies::flyer
