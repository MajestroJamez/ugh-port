#include "enemies/walker/Walking.hpp"

#include "enemies/walker/Walker.hpp"
#include "enemies/walker/Watching.hpp"

namespace ugh::enemies::walker {

namespace {

constexpr int WIDTH = 32;   // pixels: it turns before its right edge passes the end of the pad

}  // namespace

const Walking Walking::instance{};

void Walking::enter(Walker& walker, const EnemyContext&) const { walker.restartAnimation(); }

void Walking::update(Walker& walker, const EnemyContext& context) const {
    if (walker.animate()) {
        const data::levels::PadDefinition& pad = walker.pad().place();
        walker.moveToX(walker.x() + walker.speedX());
        int x = walker.x().pixels();
        if (x < pad.left || x + WIDTH >= pad.right) walker.turnAround();
        walker.showFacing(walker.kind().walk);
    }
    if (context.level.copters().landedOn(walker.pad())) {
        walker.changeState(Watching::instance, context);
        return;
    }
    stunnedByPassenger(walker, context);
}

}  // namespace ugh::enemies::walker
