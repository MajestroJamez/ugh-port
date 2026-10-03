#include "enemies/flyer/FlyerInit.hpp"

#include "enemies/flyer/FlyerWait.hpp"

namespace ugh::enemies {

const FlyerInit FlyerInit::instance{};

/** 113b:2379 */
void FlyerInit::update(model::Enemy& enemy, model::Level& level) const {
    enemy.changeState(FlyerWait::instance, level);
}

}  // namespace ugh::enemies
