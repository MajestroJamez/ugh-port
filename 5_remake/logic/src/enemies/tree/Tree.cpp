#include "enemies/tree/Tree.hpp"

#include "enemies/tree/Placed.hpp"

namespace ugh::enemies::tree {

Tree::Tree(int index, const data::TreeKind& kind, const data::TreePlacement& placement)
    : Enemy(index, placement.x, placement.y),
      StateMachine(Placed::instance),
      kind_(&kind),
      drops_(&placement.drops) {}

void Tree::update(const EnemyContext& context) { updateState(context); }

void Tree::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

}  // namespace ugh::enemies::tree
