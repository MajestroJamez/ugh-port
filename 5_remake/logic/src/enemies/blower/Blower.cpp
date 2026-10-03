#include "enemies/blower/Blower.hpp"

#include "enemies/blower/Placed.hpp"

namespace ugh::enemies::blower {

Blower::Blower(int index, const data::BlowerKind& kind, const data::BlowerPlacement& placement)
    : Enemy(index), kind_(&kind), state_(&Placed::instance) {
    x_ = placement.x();
    y_ = placement.y();
}

void Blower::update(const EnemyContext& context) { state_->update(*this, context); }

void Blower::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Blower::changeState(const BlowerState& next, const EnemyContext& context) {
    state_ = &next;
    next.enter(*this, context);
}

void Blower::continueIn(const BlowerState& next, const EnemyContext& context) {
    changeState(next, context);
    next.update(*this, context);
}

}  // namespace ugh::enemies::blower
