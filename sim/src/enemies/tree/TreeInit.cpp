#include "enemies/tree/TreeInit.hpp"

#include "enemies/tree/TreeSwaying.hpp"

namespace ugh::enemies {

const TreeInit TreeInit::instance{};

/** 113b:2a87 - the swaying starts from its first frame (after a rest it goes on where it was). */
void TreeInit::update(model::Enemy& enemy, model::Level& level) const {
    enemy.restartAnimation();
    enemy.changeState(TreeSwaying::instance, level);
}

}  // namespace ugh::enemies
