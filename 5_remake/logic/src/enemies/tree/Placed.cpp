#include "enemies/tree/Placed.hpp"

#include "enemies/tree/Swaying.hpp"
#include "enemies/tree/Tree.hpp"

namespace ugh::enemies::tree {

const Placed Placed::instance{};

void Placed::update(Tree& tree, const EnemyContext& context) const {
    tree.restartAnimation();
    tree.changeState(Swaying::instance, context);
}

}  // namespace ugh::enemies::tree
