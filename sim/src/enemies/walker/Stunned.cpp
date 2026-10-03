#include "enemies/walker/Stunned.hpp"

#include "enemies/walker/WalkerInit.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word STUNNED_TIME = 0x15e;
constexpr core::Word FRAME_DELAY = 4;

}  // namespace

const Stunned Stunned::instance{};

/** 113b:28ee - a passenger fell on it. */
void Stunned::enter(model::Enemy& enemy, model::Level& level) const {
    scoreStun(enemy, level);
    enemy.restartAnimation();
    enemy.timer().startCountdown(STUNNED_TIME);
}

/** 113b:2914 */
void Stunned::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.timer().tick()) {
        enemy.continueIn(WalkerInit::instance, level);
        return;
    }
    if (enemy.animate(FRAME_DELAY)) enemy.showFacing(enemy.kind().stunned);
}

}  // namespace ugh::enemies
