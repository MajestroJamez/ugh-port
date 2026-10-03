#include "enemies/flyer/FlyerBehavior.hpp"

#include "enemies/flyer/FlyerInit.hpp"

namespace ugh::enemies {

const FlyerBehavior FlyerBehavior::instance{};

void FlyerBehavior::place(model::Enemy& enemy, const data::EnemyPlacement& placement) const {
    enemy.setStartDelay(placement.startDelay);
    enemy.setSpeedX(placement.vx);
    enemy.facing().beforeFirstFlight();
    enemy.place(*placement.kind, FlyerInit::instance);
}

}  // namespace ugh::enemies
