#include "enemies/walker/Charging.hpp"

#include "enemies/walker/Recovering.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word FRAME_DELAY = 4;

}  // namespace

const Charging Charging::instance{};

/** 113b:272e */
void Charging::enter(model::Enemy& enemy, model::Level&) const {
    enemy.restartAnimation();
    enemy.timer().startFromStandstill();
}

/** 113b:2748 - charges at the copter, faster and faster; hitting it throws the copter into the air. */
void Charging::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.animate(FRAME_DELAY)) enemy.showFacing(enemy.kind().charging);
    if (stunnedByPassenger(enemy, level)) return;
    int copter = level.copterLandedOn(enemy.pad());
    if (copter == model::Level::NONE) {
        enemy.changeState(Recovering::instance, level);
        return;
    }
    enemy.turnTo(level.copter(copter));
    enemy.timer().speedUpBy(enemy.facing().direction());
    enemy.moveToX(enemy.x() + (enemy.timer().speed() + enemy.speedX()));
    int hit = physics::TouchBox(enemy.kind().box, enemy.x(), enemy.y()).firstCopterIn(level);
    if (hit == model::Level::NONE) return;
    level.copter(hit).throwUp(enemy.speedX().raw() + enemy.timer().speed().raw());
    enemy.changeState(Recovering::instance, level);
}

}  // namespace ugh::enemies
