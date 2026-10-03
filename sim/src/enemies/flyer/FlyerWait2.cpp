#include "enemies/flyer/FlyerWait2.hpp"

#include "enemies/flyer/Flying.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word SCREECH_TIME = 0x46;

}  // namespace

const FlyerWait2 FlyerWait2::instance{};

/** 113b:23b0 */
void FlyerWait2::enter(model::Enemy& enemy, model::Level& level) const {
    enemy.timer().startCountdown(SCREECH_TIME);
    level.report({core::EventKind::FlyerScreech, core::Event::NONE, enemy.index()});
}

/** 113b:23d9 */
void FlyerWait2::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.timer().tick()) enemy.changeState(Flying::instance, level);
}

}  // namespace ugh::enemies
