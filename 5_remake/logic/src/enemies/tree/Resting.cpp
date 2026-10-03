#include "enemies/tree/Resting.hpp"

#include "enemies/tree/Bare.hpp"
#include "enemies/tree/Swaying.hpp"
#include "enemies/tree/Tree.hpp"

namespace ugh::enemies::tree {

namespace {

constexpr units::Int16 REST_TIME = 210;

}  // namespace

const Resting Resting::instance{};

void Resting::enter(Tree& tree, const EnemyContext& context) const {
    tree.restTime().start(REST_TIME);
    tree.showSprite(context.play.data.sprites().shakenTree);
}

/** It sways on from the frame where it was. */
void Resting::update(Tree& tree, const EnemyContext& context) const {
    if (!tree.restTime().tick()) return;
    if (tree.hasDrops()) tree.changeState(Swaying::instance, context);
    else tree.changeState(Bare::instance, context);
}

}  // namespace ugh::enemies::tree
