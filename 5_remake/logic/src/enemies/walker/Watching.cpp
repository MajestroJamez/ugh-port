#include "enemies/walker/Watching.hpp"

#include "enemies/walker/Charging.hpp"
#include "enemies/walker/Placed.hpp"
#include "enemies/walker/Walker.hpp"

namespace ugh::enemies::walker {

namespace {

constexpr units::Int16 WATCH_TIME = 140;
constexpr units::Int16 FRAME_DELAY = 5;

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
    std::optional<int> copter = context.play.level.copterLandedOn(walker.pad());
    if (!copter) {
        walker.continueIn(Placed::instance, context);
        return;
    }
    walker.turnTo(context.play.level.copter(*copter));
}

}  // namespace ugh::enemies::walker
