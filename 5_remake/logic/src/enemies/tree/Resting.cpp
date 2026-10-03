#include "enemies/tree/Resting.hpp"

#include "enemies/tree/Bare.hpp"
#include "enemies/tree/Swaying.hpp"
#include "enemies/tree/Tree.hpp"

namespace ugh::enemies::tree {

namespace {

constexpr int REST_TIME = 210;

}  // namespace

const Resting Resting::instance{};

void Resting::enter(Tree& tree, const EnemyContext& context) const {
    tree.startResting(REST_TIME);
    tree.showSprite(context.data.sprites().shakenTree);
}

/** It sways on from the frame where it was. */
void Resting::update(Tree& tree, const EnemyContext& context) const {
    if (!tree.tickRest()) return;
    if (tree.hasDrops()) tree.changeState(Swaying::instance, context);
    else tree.changeState(Bare::instance, context);
}

}  // namespace ugh::enemies::tree
