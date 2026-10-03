#include "enemies/walker/Stunned.hpp"

#include "enemies/walker/Placed.hpp"
#include "enemies/walker/Walker.hpp"

namespace ugh::enemies::walker {

const Stunned Stunned::instance{};

void Stunned::enter(Walker& walker, const EnemyContext& context) const {
    walker.scoreStun(walker.kind().score, context);
    walker.restartAnimation();
    walker.stunTime().start(STUN_TIME);
}

void Stunned::update(Walker& walker, const EnemyContext& context) const {
    if (walker.stunTime().tick()) {
        walker.continueIn(Placed::instance, context);
        return;
    }
    if (walker.animate()) walker.showFacing(walker.kind().stunned);
}

}  // namespace ugh::enemies::walker
