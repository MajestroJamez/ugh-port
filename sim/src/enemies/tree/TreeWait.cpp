#include "enemies/tree/TreeWait.hpp"

#include "enemies/tree/Inactive.hpp"
#include "enemies/tree/TreeSwaying.hpp"

namespace ugh::enemies {

namespace {

constexpr data::Sprite HIT_SPRITE = 0xe8;
constexpr core::Word REST_TIME = 0xd2;

}  // namespace

const TreeWait TreeWait::instance{};

/** 113b:2b0c - a passenger bounced off it. */
void TreeWait::enter(model::Enemy& enemy, model::Level&) const {
    enemy.timer().startCountdown(REST_TIME);
    enemy.showSprite(HIT_SPRITE);
}

/** 113b:2b58 - rests, then sways again, or stays still when its bonus items are all gone. */
void TreeWait::update(model::Enemy& enemy, model::Level& level) const {
    if (!enemy.timer().tick()) return;
    const data::DropCursor* drops = enemy.table().dropCursor();
    if (drops && !drops->finished()) enemy.changeState(TreeSwaying::instance, level);
    else enemy.changeState(Inactive::instance, level);
}

}  // namespace ugh::enemies
