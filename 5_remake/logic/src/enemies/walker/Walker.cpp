#include "enemies/walker/Walker.hpp"

#include "enemies/walker/Placed.hpp"

namespace ugh::enemies::walker {

Walker::Walker(int index, const data::kinds::WalkerKind& kind, const data::levels::WalkerPlacement& placement,
               world::scenery::Pad& pad)
    : Enemy(index, placement.x, placement.y),
      StateMachine(Placed::instance),
      kind_(&kind),
      pad_(&pad),
      vx_(placement.speed) {}

void Walker::update(const EnemyContext& context) { updateState(context); }

void Walker::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Walker::turnAround() {
    vx_ = -vx_;
    facing_ = facing_ == world::figure::Facing::Left ? world::figure::Facing::Right : world::figure::Facing::Left;
}

void Walker::turnTo(const world::copter::Copter& copter) {
    facing_ = x() < copter.motion().x() ? world::figure::Facing::Right : world::figure::Facing::Left;
    vx_ = headed(vx_, facing_);
}

}  // namespace ugh::enemies::walker
