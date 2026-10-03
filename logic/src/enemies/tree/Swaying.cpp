#include "enemies/tree/Swaying.hpp"

#include "enemies/tree/Resting.hpp"
#include "enemies/tree/Tree.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::enemies::tree {

namespace {

constexpr units::Int16 FRAME_DELAY = 6;

}  // namespace

const Swaying Swaying::instance{};

void Swaying::update(Tree& tree, const EnemyContext& context) const {
    if (!tree.animate(FRAME_DELAY)) return;
    tree.show(*tree.kind().swaying);
    passengers::standing::StandingPassenger* passenger = context.passengers.fallingOnto(tree.x(), tree.y());
    if (!passenger) return;
    passenger->bounce((-passenger->fallSpeed()) >> 1, context.play.data.sprites().bouncedPassenger);
    tree.changeState(Resting::instance, context);
    if (!tree.hasDrops()) {
        context.play.diagnostics.report("a tree without bonus items to drop");
        return;
    }
    // up with a quarter of the bounce
    context.bonuses.drop(tree.takeDrop(), passenger->x(), passenger->y(), passenger->dropSpeedX(),
                         (-passenger->fallSpeed()) >> 2, context.play.diagnostics);
    context.play.report({events::EventKind::TreeDrop, std::nullopt, tree.index()});
}

}  // namespace ugh::enemies::tree
