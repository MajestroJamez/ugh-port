#include "enemies/walker/Walking.hpp"

#include "enemies/walker/Watching.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word STEP_DELAY = 4;
constexpr core::Word WIDTH = 0x20;   // pixels: it turns before its right edge passes the pad's

}  // namespace

const Walking Walking::instance{};

/** 113b:25b1 */
void Walking::enter(model::Enemy& enemy, model::Level&) const { enemy.restartAnimation(); }

/** 113b:25c9 - walks along its pad, a step per animation frame, turning at the ends; watches a copter landing there. */
void Walking::update(model::Enemy& enemy, model::Level& level) const {
    if (enemy.animate(STEP_DELAY)) {
        const model::Pad& pad = level.pad(enemy.pad());
        enemy.moveToX(enemy.x() + enemy.speedX());
        core::Word x = enemy.x().pixels();
        if (x < pad.left() || x + WIDTH >= pad.right()) {
            enemy.reverse();
            enemy.facing().turnAround();
        }
        enemy.showFacing(enemy.kind().moving);
    }
    if (level.copterLandedOn(enemy.pad()) != model::Level::NONE) {
        enemy.changeState(Watching::instance, level);
        return;
    }
    stunnedByPassenger(enemy, level);
}

}  // namespace ugh::enemies
