#include "enemies/walker/Watching.hpp"

#include "enemies/walker/Charging.hpp"
#include "enemies/walker/WalkerInit.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word WATCH_TIME = 0x8c;
constexpr core::Word FRAME_DELAY = 5;

}  // namespace

const Watching Watching::instance{};

/** 113b:2667 - a copter landed on its pad. */
void Watching::enter(model::Enemy& enemy, model::Level&) const {
    enemy.restartAnimation();
    enemy.timer().startCountdown(WATCH_TIME);
}

/** 113b:2681 - looks at the landed copter for a while, then charges; walks on when it took off. */
void Watching::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.timer().tick()) {
        enemy.changeState(Charging::instance, level);
        return;
    }
    if (enemy.animate(FRAME_DELAY)) enemy.showFacing(enemy.kind().watching);
    if (stunnedByPassenger(enemy, level)) return;
    int copter = level.copterLandedOn(enemy.pad());
    if (copter == model::Level::NONE) {
        enemy.continueIn(WalkerInit::instance, level);
        return;
    }
    enemy.turnTo(level.copter(copter));
}

}  // namespace ugh::enemies
