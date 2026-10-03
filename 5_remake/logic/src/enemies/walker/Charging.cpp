#include "enemies/walker/Charging.hpp"

#include "enemies/walker/Recovering.hpp"
#include "enemies/walker/Walker.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::enemies::walker {

const Charging Charging::instance{};

void Charging::enter(Walker& walker, const EnemyContext&) const {
    walker.restartAnimation();
    walker.startCharge();
}

void Charging::update(Walker& walker, const EnemyContext& context) const {
    world::Level& level = context.play.level;
    if (walker.animate()) walker.showFacing(walker.kind().charge);
    if (stunnedByPassenger(walker, context)) return;
    std::optional<int> copter = level.copters().landedOn(walker.pad());
    if (!copter) {
        walker.changeState(Recovering::instance, context);
        return;
    }
    walker.turnTo(level.copters()[*copter]);
    walker.chargeFaster();
    walker.moveToX(walker.x() + (units::Fixed::fromRaw(walker.chargeSpeed()) + walker.speedX()));
    std::optional<int> hit = physics::TouchBox(walker.kind().box, walker.x(), walker.y()).firstCopterIn(level);
    if (!hit) return;
    level.copters()[*hit].throwUp(walker.speedX().raw() + walker.chargeSpeed());
    walker.changeState(Recovering::instance, context);
}

}  // namespace ugh::enemies::walker
