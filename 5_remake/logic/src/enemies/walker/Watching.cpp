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
    walker.watchTime().start(WATCH_TIME);
}

void Watching::update(Walker& walker, const EnemyContext& context) const {
    if (walker.watchTime().tick()) {
        walker.changeState(Charging::instance, context);
        return;
    }
    if (walker.animate(FRAME_DELAY)) walker.showFacing(walker.kind().watch);
    if (stunnedByPassenger(walker, context)) return;
    std::optional<int> copter = context.play.level.copters().landedOn(walker.pad());
    if (!copter) {
        walker.continueIn(Placed::instance, context);
        return;
    }
    walker.turnTo(context.play.level.copters()[*copter]);
}

}  // namespace ugh::enemies::walker
