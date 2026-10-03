#include "enemies/tree/Swaying.hpp"

#include "enemies/tree/Resting.hpp"
#include "enemies/tree/Tree.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::enemies::tree {

namespace {

constexpr int FRAME_DELAY = 6;

}  // namespace

const Swaying Swaying::instance{};

void Swaying::update(Tree& tree, const EnemyContext& context) const {
    if (!tree.animate(FRAME_DELAY)) return;
    tree.show(*tree.kind().swaying);
    const passengers::standing::StandingPassenger* passenger =
        tree.bounceFallingPassenger(context, Enemy::Rebound::Half);
    if (!passenger) return;
    tree.changeState(Resting::instance, context);
    if (!tree.hasDrops()) {
        context.diagnostics.report("a tree without bonus items to drop");
        return;
    }
    // up with a quarter of the bounce
    context.bonuses.drop(tree.takeDrop(), passenger->x(), passenger->y(), passenger->dropSpeedX(),
                         (-passenger->fallSpeed()).half().half(), context.diagnostics);
    context.report({events::EventKind::TreeDrop, std::nullopt, tree.index()});
}

}  // namespace ugh::enemies::tree
