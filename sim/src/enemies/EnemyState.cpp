#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

void EnemyState::enter(model::Enemy&, model::Level&) const {}

bool EnemyState::bounceFallingPassenger(model::Enemy& enemy, model::Level& level, bool showHit) {
    int i = level.fallingPassengerNear(enemy);
    if (i == model::Level::NONE) return false;
    model::Passenger& passenger = level.passenger(i);
    passenger.setFallSpeed(-passenger.fallSpeed());
    if (showHit) passenger.showSprite(HIT_PASSENGER_SPRITE);
    return true;
}

void EnemyState::scoreStun(const model::Enemy& enemy, model::Level& level) {
    core::Word score = enemy.kind().score;
    level.session().addScore(score.bits());
    level.report({core::EventKind::EnemyStunned, core::Event::NONE, enemy.index(), score.value()});
}

}  // namespace ugh::enemies
