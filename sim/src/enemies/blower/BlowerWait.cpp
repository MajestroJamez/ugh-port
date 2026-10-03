#include "enemies/blower/BlowerWait.hpp"

#include "enemies/blower/BlowerInit.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word STUNNED_TIME = 0x15e;

}  // namespace

const BlowerWait BlowerWait::instance{};

/** 113b:2a53 - a passenger fell on it. */
void BlowerWait::enter(model::Enemy& enemy, model::Level& level) const {
    scoreStun(enemy, level);
    enemy.timer().startCountdown(STUNNED_TIME);
    enemy.showSprite(enemy.kind().stunned.left->frame(0));
}

/** 113b:2a76 */
void BlowerWait::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.timer().tick()) enemy.continueIn(BlowerInit::instance, level);
}

}  // namespace ugh::enemies
