#include "enemies/walker/Watching.hpp"

#include "enemies/walker/Charging.hpp"
#include "enemies/walker/Placed.hpp"
#include "enemies/walker/Walker.hpp"

namespace ugh::enemies::walker {

namespace {

constexpr int WATCH_TIME = 140;
constexpr int FRAME_DELAY = 5;   // slower than its other animations

}  // namespace

const Watching Watching::instance{};

void Watching::enter(Walker& walker, const EnemyContext&) const {
    walker.restartAnimation();
    walker.startWatching(WATCH_TIME);
}

void Watching::update(Walker& walker, const EnemyContext& context) const {
    if (walker.watchOver()) {
        walker.changeState(Charging::instance, context);
        return;
    }
    if (walker.animate(FRAME_DELAY)) walker.showFacing(walker.kind().watch);
    if (stunnedByPassenger(walker, context)) return;
    const world::Copter* copter = context.level.copters().landedOn(walker.pad());
    if (!copter) {
        walker.continueIn(Placed::instance, context);
        return;
    }
    walker.turnTo(*copter);
}

}  // namespace ugh::enemies::walker
