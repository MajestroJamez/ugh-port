#include "enemies/blower/Blower.hpp"

#include "enemies/blower/Placed.hpp"

namespace ugh::enemies::blower {

Blower::Blower(int index, const data::kinds::BlowerKind& kind, const data::levels::BlowerPlacement& placement)
    : Enemy(index, placement.x, placement.y), StateMachine(Placed::instance), kind_(&kind) {}

void Blower::update(const EnemyContext& context) { updateState(context); }

void Blower::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

}  // namespace ugh::enemies::blower
