#include "enemies/walker/Charging.hpp"

#include "enemies/walker/Recovering.hpp"
#include "enemies/walker/Walker.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::enemies::walker {

const Charging Charging::instance{};

void Charging::enter(Walker& walker, const EnemyContext&) const {
    walker.restartAnimation();
    walker.charge().start();
}

void Charging::update(Walker& walker, const EnemyContext& context) const {
    world::Level& level = context.level;
    if (walker.animate()) walker.showFacing(walker.kind().charge);
    if (stunnedByPassenger(walker, context)) return;
    const world::copter::Copter* copter = level.copters().landedOn(walker.pad());
    if (!copter) {
        walker.changeState(Recovering::instance, context);
        return;
    }
    walker.turnTo(*copter);
    walker.charge().faster(walker.facing());
    walker.moveToX(walker.x() + walker.chargeSpeed());
    world::copter::Copter* hit =
        physics::TouchBox(walker.kind().box, walker.x(), walker.y()).firstCopterIn(level.copters());
    if (!hit) return;
    hit->throwUp(walker.chargeSpeed());
    walker.changeState(Recovering::instance, context);
}

}  // namespace ugh::enemies::walker
