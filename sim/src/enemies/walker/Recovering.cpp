#include "enemies/walker/Recovering.hpp"

#include "enemies/walker/WalkerInit.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word FRAME_DELAY = 4;

}  // namespace

const Recovering Recovering::instance{};

/** 113b:2830 / 288f - after charging (the copter left, or it was thrown). */
void Recovering::enter(model::Enemy& enemy, model::Level&) const { enemy.restartAnimation(); }

/** 113b:2844 - one run of the animation, then walking again. */
void Recovering::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.animate(FRAME_DELAY)) {
        const data::Animation& animation = enemy.kind().recovering.towards(enemy.facing().right());
        if (enemy.pastEndOf(animation)) {
            enemy.continueIn(WalkerInit::instance, level);
            return;
        }
        enemy.showFrameOf(animation);
    }
    stunnedByPassenger(enemy, level);
}

}  // namespace ugh::enemies
