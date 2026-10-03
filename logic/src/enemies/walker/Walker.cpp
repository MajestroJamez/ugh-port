#include "enemies/walker/Walker.hpp"

#include "enemies/walker/Placed.hpp"

namespace ugh::enemies::walker {

Walker::Walker(int index, const data::WalkerKind& kind, const data::WalkerPlacement& placement)
    : Enemy(index), kind_(&kind), state_(&Placed::instance), pad_(placement.pad()), vx_(placement.speed()) {
    x_ = placement.x();
    y_ = placement.y();
}

void Walker::update(const EnemyContext& context) { state_->update(*this, context); }

void Walker::accept(EnemyVisitor& visitor) const { visitor.visit(*this); }

void Walker::changeState(const WalkerState& next, const EnemyContext& context) {
    state_ = &next;
    next.enter(*this, context);
}

void Walker::continueIn(const WalkerState& next, const EnemyContext& context) {
    changeState(next, context);
    next.update(*this, context);
}

void Walker::turnAround() {
    vx_ = -vx_;
    facing_ = facing_ == world::Facing::Left ? world::Facing::Right : world::Facing::Left;
}

void Walker::turnTo(const world::Copter& copter) {
    if (x_ < copter.x()) {
        facing_ = world::Facing::Right;
        headRight();
    } else {
        facing_ = world::Facing::Left;
        headLeft();
    }
}

void Walker::headLeft() {
    if (vx_ >= units::Fixed()) vx_ = -vx_;
}

void Walker::headRight() {
    if (vx_ < units::Fixed()) vx_ = -vx_;
}

}  // namespace ugh::enemies::walker
