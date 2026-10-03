#include "enemies/flyer/Flying.hpp"

#include <string>

#include "enemies/flyer/FlyerFalling.hpp"
#include "enemies/flyer/FlyerInit.hpp"

namespace ugh::enemies {

namespace {

using core::Fixed;

constexpr Fixed SCREEN_MIDDLE(0x1400);
constexpr Fixed START_RIGHT(0x27e0), START_LEFT(-0x3e0);   // just off the screen
constexpr Fixed LEFT_EDGE(-0x400), RIGHT_EDGE(0x2800);     // gone past these
constexpr Fixed HEIGHT(0x340);                             // it flies no lower than this above the water
constexpr Fixed CEILING(-0x80);                            // and no higher than this
constexpr core::Word FLAP_DELAY = 4;

}  // namespace

const Flying Flying::instance{};

/** 113b:23ea - takes the other player as its target (in the team mode) and comes in from the far side at its height. */
void Flying::enter(model::Enemy& enemy, model::Level& level) const {
    int target = enemy.facing().targetNextPlayer(level.session().players());
    if (target != 0 && target != 1) {
        level.diagnostics().report("a flyer hunts player " + std::to_string(target));
        target = 0;
    }
    const model::Copter& copter = level.copter(target);
    const data::EnemyKind& kind = enemy.kind();
    if (copter.x() < SCREEN_MIDDLE) {   // the copter is on the left: in from the right edge
        enemy.headLeft();
        enemy.moveToX(START_RIGHT);
        enemy.setTable(model::EnemyTable::flight(kind.moving.left));
    } else {
        enemy.headRight();
        enemy.moveToX(START_LEFT);
        enemy.setTable(model::EnemyTable::flight(kind.moving.right));
    }
    Fixed y = copter.y() + HEIGHT;
    if (y > level.water().level()) y = level.water().level();
    y -= HEIGHT;
    if (y < CEILING) y = CEILING;
    enemy.moveToY(y);
    level.report({core::EventKind::FlyerFlapStart, core::Event::NONE, enemy.index()});
}

/** 113b:2493 - flies across the screen; touching its target's copter ends the attempt. */
void Flying::update(model::Enemy& enemy, model::Level& level) const {
    Fixed x = enemy.x() + enemy.speedX();
    if (x <= LEFT_EDGE || x >= RIGHT_EDGE) {
        level.report({core::EventKind::FlyerFlapStop, core::Event::NONE, enemy.index()});
        enemy.continueIn(FlyerInit::instance, level);
        return;
    }
    enemy.moveToX(x);
    if (!enemy.animate(FLAP_DELAY)) return;
    if (const data::Animation* flight = enemy.table().flightAnimation()) enemy.show(*flight);
    else level.diagnostics().report("a flyer without its flight animation");
    if (bounceFallingPassenger(enemy, level, true)) {
        level.report({core::EventKind::FlyerFlapStop, core::Event::NONE, enemy.index()});
        enemy.changeState(FlyerFalling::instance, level);
        return;
    }
    int copter = level.copterTouching(enemy.kind().box, enemy.x(), enemy.y());
    if (copter == model::Level::NONE || copter != enemy.facing().target() || level.fade().fadingOut()) return;
    level.fade().startFadeOut();
    level.report({core::EventKind::CopterCrashed, copter});
}

}  // namespace ugh::enemies
