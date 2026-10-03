#include "enemies/walker/Walker.hpp"

#include "enemies/walker/Placed.hpp"

namespace ugh::enemies::walker {

Walker::Walker(int index, const data::WalkerKind& kind, const data::WalkerPlacement& placement)
    : Enemy(index, placement.x, placement.y),
      StateMachine(Placed::instance),
      kind_(&kind),
      pad_(placement.pad),
      vx_(placement.speed) {}

void Walker::update(const EnemyContext& context) { updateState(context); }

void Walker::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Walker::turnAround() {
    vx_ = -vx_;
    facing_ = facing_ == world::Facing::Left ? world::Facing::Right : world::Facing::Left;
}

void Walker::turnTo(const world::Copter& copter) {
    facing_ = x() < copter.x() ? world::Facing::Right : world::Facing::Left;
    vx_ = headed(vx_, facing_);
}

}  // namespace ugh::enemies::walker
