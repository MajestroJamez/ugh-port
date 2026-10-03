#include "enemies/walker/WalkerBehavior.hpp"

#include "enemies/walker/WalkerInit.hpp"

namespace ugh::enemies {

const WalkerBehavior WalkerBehavior::instance{};

void WalkerBehavior::place(model::Enemy& enemy, const data::EnemyPlacement& placement) const {
    enemy.placeOnPad(placement.pad);
    enemy.moveTo(placement.x, placement.y);
    enemy.setSpeedX(placement.vx);
    enemy.facing().faceLeft();
    enemy.place(*placement.kind, WalkerInit::instance);
}

}  // namespace ugh::enemies
