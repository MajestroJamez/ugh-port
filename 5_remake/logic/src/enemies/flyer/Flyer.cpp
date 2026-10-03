#include "enemies/flyer/Flyer.hpp"

#include "enemies/flyer/Placed.hpp"

namespace ugh::enemies::flyer {

Flyer::Flyer(int index, const data::kinds::FlyerKind& kind, const data::levels::FlyerPlacement& placement)
    : Enemy(index),
      StateMachine(Placed::instance),
      kind_(&kind),
      startDelay_(placement.startDelay),
      vx_(placement.speed) {}

void Flyer::update(const EnemyContext& context) { updateState(context); }

void Flyer::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Flyer::flyTowards(world::figure::Facing side) {
    vx_ = headed(vx_, side);
    flight_ = side;
}

int Flyer::takeNextTarget(int players) {
    int target = lastTarget_ ^ 1;
    if (target >= players) target = 0;
    lastTarget_ = target;
    return target;
}

}  // namespace ugh::enemies::flyer
