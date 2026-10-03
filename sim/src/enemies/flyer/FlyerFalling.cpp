#include "enemies/flyer/FlyerFalling.hpp"

#include "enemies/flyer/FlyerInit.hpp"

namespace ugh::enemies {

namespace {

using core::Fixed;

constexpr Fixed LEFT_EDGE(-0x400), RIGHT_EDGE(0x2800), BOTTOM_EDGE(0x1800);   // gone past these

}  // namespace

const FlyerFalling FlyerFalling::instance{};

/** 113b:252b - a passenger fell on it. */
void FlyerFalling::enter(model::Enemy& enemy, model::Level& level) const {
    scoreStun(enemy, level);
    enemy.timer().startFromStandstill();
    const data::AnimationPair& hit = enemy.kind().stunned;
    enemy.showSprite((enemy.speedX() < Fixed(0) ? hit.left : hit.right)->frame(0));
}

/** 113b:255e - falls down, faster and faster, until it is off the screen; then it starts again. */
void FlyerFalling::update(model::Enemy& enemy, model::Level& level) const {
    Fixed x = enemy.speedX() + enemy.x();
    if (x <= LEFT_EDGE || x >= RIGHT_EDGE) {
        enemy.continueIn(FlyerInit::instance, level);
        return;
    }
    enemy.moveToX(x);
    enemy.timer().fallFaster();
    Fixed y = enemy.timer().speed() + enemy.y();
    if (y >= BOTTOM_EDGE) {
        enemy.continueIn(FlyerInit::instance, level);
        return;
    }
    enemy.moveToY(y);
}

}  // namespace ugh::enemies
