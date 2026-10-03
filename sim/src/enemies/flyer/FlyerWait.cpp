#include "enemies/flyer/FlyerWait.hpp"

#include "enemies/flyer/FlyerWait2.hpp"

namespace ugh::enemies {

const FlyerWait FlyerWait::instance{};

/** 113b:2379 */
void FlyerWait::enter(model::Enemy& enemy, model::Level&) const {
    enemy.restartAnimation();
    enemy.hide();
    enemy.timer().startCountdown(enemy.startDelay());
}

/** 113b:239f */
void FlyerWait::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.timer().tick()) enemy.changeState(FlyerWait2::instance, level);
}

}  // namespace ugh::enemies
