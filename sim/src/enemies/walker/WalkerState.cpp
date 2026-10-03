#include "enemies/walker/WalkerState.hpp"

#include "enemies/walker/Stunned.hpp"

namespace ugh::enemies {

bool WalkerState::stunnedByPassenger(model::Enemy& enemy, model::Level& level) {
    if (!bounceFallingPassenger(enemy, level, true)) return false;
    enemy.changeState(Stunned::instance, level);
    return true;
}

}  // namespace ugh::enemies
