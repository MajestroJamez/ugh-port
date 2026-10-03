#include "bonuses/Lying.hpp"

#include "physics/TouchBox.hpp"

namespace ugh::bonuses {

namespace {

constexpr units::Int16 LYING_TIME = 560;   // frames
constexpr units::Int16 TOUCH_HALF_SIZE = 3;   // pixels: a copter touches the 6 x 6 px around its anchor

}  // namespace

const Lying Lying::instance{};

void Lying::start(BonusItem& item) {
    item.changeState(instance);
    item.startLying(LYING_TIME);
}

void Lying::update(BonusItem& item, const world::PlayContext& context) const {
    if (item.lyingOver()) {
        item.disappear();
        return;
    }
    const data::BonusKind& kind = item.kind();
    physics::TouchBox box({kind.anchorX, kind.anchorY, TOUCH_HALF_SIZE, TOUCH_HALF_SIZE}, item.x(), item.y());
    std::optional<int> copter = box.firstCopterIn(context.level);
    if (!copter) return;
    switch (kind.effect) {
        case data::BonusEffect::Energy: context.level.energy().refill(kind.amount); break;
        case data::BonusEffect::Life: context.session.addLives(kind.amount); break;
        case data::BonusEffect::Multiplier: context.session.raiseMultiplier(); break;
    }
    item.disappear();
    context.report({events::EventKind::BonusCollected, *copter, item.slot(), static_cast<int>(kind.effect)});
}

}  // namespace ugh::bonuses
