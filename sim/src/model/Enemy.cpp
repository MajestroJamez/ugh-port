#include "model/Enemy.hpp"

#include "enemies/EnemyState.hpp"
#include "model/Copter.hpp"

namespace ugh::model {

void Enemy::place(const data::EnemyKind& kind, const enemies::EnemyState& state) {
    s_.kind = &kind;
    s_.state = &state;
}

void Enemy::moveTo(core::Fixed x, core::Fixed y) {
    s_.x = x;
    s_.y = y;
}

void Enemy::update(Level& level) { s_.state->update(*this, level); }

void Enemy::changeState(const enemies::EnemyState& next, Level& level) {
    s_.state = &next;
    next.enter(*this, level);
}

void Enemy::continueIn(const enemies::EnemyState& next, Level& level) {
    changeState(next, level);
    next.update(*this, level);
}

void Enemy::headLeft() {
    if (s_.vx >= core::Fixed(0)) s_.vx = -s_.vx;
}

void Enemy::headRight() {
    if (s_.vx < core::Fixed(0)) s_.vx = -s_.vx;
}

void Enemy::turnTo(const Copter& copter) {
    if (s_.x < copter.x()) {
        s_.facing.faceRight();
        headRight();
    } else {
        s_.facing.faceLeft();
        headLeft();
    }
}

}  // namespace ugh::model
