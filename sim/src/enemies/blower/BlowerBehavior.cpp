#include "enemies/blower/BlowerBehavior.hpp"

#include "enemies/blower/BlowerInit.hpp"

namespace ugh::enemies {

const BlowerBehavior BlowerBehavior::instance{};

void BlowerBehavior::place(model::Enemy& enemy, const data::EnemyPlacement& placement) const {
    enemy.moveTo(placement.x, placement.y);
    enemy.place(*placement.kind, BlowerInit::instance);
}

}  // namespace ugh::enemies
