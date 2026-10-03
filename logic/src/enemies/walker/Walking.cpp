#include "enemies/walker/Walking.hpp"

#include "enemies/walker/Walker.hpp"
#include "enemies/walker/Watching.hpp"

namespace ugh::enemies::walker {

namespace {

constexpr units::Int16 STEP_DELAY = 4;
constexpr units::Int16 WIDTH = 32;   // pixels: it turns before its right edge passes the end of the pad

}  // namespace

const Walking Walking::instance{};

void Walking::enter(Walker& walker, const EnemyContext&) const { walker.restartAnimation(); }

void Walking::update(Walker& walker, const EnemyContext& context) const {
    if (walker.animate(STEP_DELAY)) {
        const data::PadDefinition& pad = context.play.level.pad(walker.pad()).place();
        walker.moveToX(walker.x() + walker.speedX());
        units::Int16 x = walker.x().pixels();
        if (x < pad.left || x + WIDTH >= pad.right) walker.turnAround();
        walker.showFacing(walker.kind().walk);
    }
    if (context.play.level.copterLandedOn(walker.pad())) {
        walker.changeState(Watching::instance, context);
        return;
    }
    stunnedByPassenger(walker, context);
}

}  // namespace ugh::enemies::walker
