#include "enemies/tree/TreeSwaying.hpp"

#include "bonuses/Falling.hpp"
#include "enemies/tree/TreeWait.hpp"

namespace ugh::enemies {

namespace {

constexpr core::Word FRAME_DELAY = 6;

}  // namespace

const TreeSwaying TreeSwaying::instance{};

/**
 * 113b:2ab5 - sways; a passenger falling onto it bounces off half as high, and the next bonus item of the tree's
 * list drops where it hit (113b:2b0c).
 */
void TreeSwaying::update(model::Enemy& enemy, model::Level& level) const {
    if (!enemy.animate(FRAME_DELAY)) return;
    enemy.show(*enemy.kind().moving.left);
    int i = level.fallingPassengerNear(enemy);
    if (i == model::Level::NONE) return;
    model::Passenger& passenger = level.passenger(i);
    passenger.setFallSpeed((-passenger.fallSpeed()) >> 1);
    passenger.showSprite(HIT_PASSENGER_SPRITE);
    enemy.changeState(TreeWait::instance, level);
    data::DropCursor* drops = enemy.table().dropCursor();
    if (!drops || drops->finished()) {
        level.diagnostics().report("a tree without bonus items to drop");
        return;
    }
    bonuses::Falling::drop(level, drops->next(), passenger.x(), passenger.y(), passenger.timer().dropSpeed(),
                           (-passenger.fallSpeed()) >> 2);   // up with a quarter of the bounce
    drops->index++;
    level.report({core::EventKind::TreeDrop, core::Event::NONE, enemy.index()});
}

}  // namespace ugh::enemies
