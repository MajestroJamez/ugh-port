#include "bonuses/Lying.hpp"

#include "physics/TouchBox.hpp"

namespace ugh::bonuses {

namespace {

constexpr core::Word LYING_TIME = 0x230;
constexpr core::Word TOUCH_HALF_SIZE = 3;   // pixels: the box a copter touches is 6 x 6 around the item's anchor

}  // namespace

const Lying Lying::instance{};

/** 113b:2c97 - landed. */
void Lying::enter(model::BonusItem& item, model::Level&) const { item.timer().startLying(LYING_TIME); }

/** 113b:2ca9 - lies; a copter touching it collects it: energy, a life or a higher score multiplier. */
void Lying::update(model::BonusItem& item, model::Level& level) const {
    if (item.timer().lyingOver()) {
        item.disappear();
        return;
    }
    const data::BonusKind& kind = item.kind();
    physics::TouchBox box({kind.x, kind.y, TOUCH_HALF_SIZE, TOUCH_HALF_SIZE}, item.x(), item.y());
    int copter = box.firstCopterIn(level);
    if (copter == model::Level::NONE) return;
    switch (kind.effect) {
        case data::BonusKind::Effect::Energy: level.energy().refill(kind.amount); break;
        case data::BonusKind::Effect::Life: level.session().addLives(kind.amount); break;
        case data::BonusKind::Effect::Multiplier: level.session().raiseMultiplier(); break;
    }
    item.disappear();
    level.report({core::EventKind::BonusCollected, copter, item.index(), static_cast<int>(kind.effect)});
}

}  // namespace ugh::bonuses
