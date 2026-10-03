#include "enemies/tree/TreeBehavior.hpp"

#include "enemies/tree/TreeInit.hpp"

namespace ugh::enemies {

const TreeBehavior TreeBehavior::instance{};

void TreeBehavior::place(model::Enemy& enemy, const data::EnemyPlacement& placement) const {
    enemy.placeOnPad(placement.pad);
    enemy.moveTo(placement.x, placement.y);
    enemy.holdDrops(*placement.drops);
    enemy.place(*placement.kind, TreeInit::instance);
}

}  // namespace ugh::enemies
