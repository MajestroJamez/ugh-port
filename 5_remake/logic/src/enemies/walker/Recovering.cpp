#include "enemies/walker/Recovering.hpp"

#include "enemies/walker/Placed.hpp"
#include "enemies/walker/Walker.hpp"

namespace ugh::enemies::walker {

const Recovering Recovering::instance{};

void Recovering::enter(Walker& walker, const EnemyContext&) const { walker.restartAnimation(); }

void Recovering::update(Walker& walker, const EnemyContext& context) const {
    if (walker.animate()) {
        const data::kinds::Animation& animation = walker.kind().recover.towards(walker.facing());
        if (walker.pastEndOf(animation)) {
            walker.continueIn(Placed::instance, context);
            return;
        }
        walker.showFrameOf(animation);
    }
    stunnedByPassenger(walker, context);
}

}  // namespace ugh::enemies::walker
