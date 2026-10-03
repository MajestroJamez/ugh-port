#include "enemies/walker/WalkerState.hpp"

#include "enemies/walker/Stunned.hpp"
#include "enemies/walker/Walker.hpp"

namespace ugh::enemies::walker {

bool WalkerState::stunnedByPassenger(Walker& walker, const EnemyContext& context) {
    if (!walker.bounceFallingPassenger(context)) return false;
    walker.changeState(Stunned::instance, context);
    return true;
}

}  // namespace ugh::enemies::walker
