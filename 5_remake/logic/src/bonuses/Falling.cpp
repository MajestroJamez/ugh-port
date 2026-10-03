#include "bonuses/Falling.hpp"

#include "bonuses/Lying.hpp"
#include "physics/Ballistics.hpp"

namespace ugh::bonuses {

namespace {

constexpr int GRAVITY = 3;   // 1/32 px per frame, per frame

}  // namespace

const Falling Falling::instance{};

void Falling::update(BonusItem& item, const world::PlayContext& context) const {
    const data::kinds::BonusKind& kind = item.kind();
    physics::Ballistics::Body body{item.x(), item.y(), kind.anchorX, kind.anchorY, item.speedX(), item.fallSpeed()};
    physics::Ballistics::Result result =
        physics::Ballistics(GRAVITY, physics::Ballistics::Landing::BonusItem).fall(body, context.level);
    if (result == physics::Ballistics::Result::Gone) {
        item.disappear();
        return;
    }
    item.moveTo(body.x, body.y, body.fallSpeed);
    if (result == physics::Ballistics::Result::Landed) item.changeState(Lying::instance, context);
}

}  // namespace ugh::bonuses
