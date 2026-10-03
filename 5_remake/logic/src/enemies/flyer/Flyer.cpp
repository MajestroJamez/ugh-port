#include "enemies/flyer/Flyer.hpp"

#include "enemies/flyer/Placed.hpp"

namespace ugh::enemies::flyer {

Flyer::Flyer(int index, const data::FlyerKind& kind, const data::FlyerPlacement& placement)
    : Enemy(index), kind_(&kind), state_(&Placed::instance), startDelay_(placement.startDelay()), vx_(placement.speed()) {}

void Flyer::update(const EnemyContext& context) { state_->update(*this, context); }

void Flyer::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Flyer::changeState(const FlyerState& next, const EnemyContext& context) {
    state_ = &next;
    next.enter(*this, context);
}

void Flyer::continueIn(const FlyerState& next, const EnemyContext& context) {
    changeState(next, context);
    next.update(*this, context);
}

void Flyer::headLeft() { vx_ = headed(vx_, world::Facing::Left); }

void Flyer::headRight() { vx_ = headed(vx_, world::Facing::Right); }

int Flyer::takeNextTarget(int players) {
    int target = lastTarget_ ^ 1;
    if (target >= players) target = 0;
    lastTarget_ = target;
    return target;
}

}  // namespace ugh::enemies::flyer
