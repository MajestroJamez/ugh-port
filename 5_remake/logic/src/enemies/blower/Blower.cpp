#include "enemies/blower/Blower.hpp"

#include "enemies/blower/Placed.hpp"

namespace ugh::enemies::blower {

Blower::Blower(int index, const data::BlowerKind& kind, const data::BlowerPlacement& placement)
    : Enemy(index), StateMachine(Placed::instance), kind_(&kind) {
    x_ = placement.x();
    y_ = placement.y();
}

void Blower::update(const EnemyContext& context) { updateState(context); }

void Blower::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

}  // namespace ugh::enemies::blower
