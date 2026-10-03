#include "enemies/tree/Tree.hpp"

#include "enemies/tree/Placed.hpp"

namespace ugh::enemies::tree {

Tree::Tree(int index, const data::TreeKind& kind, const data::TreePlacement& placement)
    : Enemy(index), kind_(&kind), state_(&Placed::instance), drops_(&placement.drops()) {
    x_ = placement.x();
    y_ = placement.y();
}

void Tree::update(const EnemyContext& context) { state_->update(*this, context); }

void Tree::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Tree::changeState(const TreeState& next, const EnemyContext& context) {
    state_ = &next;
    next.enter(*this, context);
}

}  // namespace ugh::enemies::tree
