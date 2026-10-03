#include "enemies/Enemy.hpp"

#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::enemies {

bool Enemy::bounceFallingPassenger(const EnemyContext& context, bool showHit) const {
    passengers::standing::StandingPassenger* passenger = context.passengers.fallingOnto(x_, y_);
    if (!passenger) return false;
    if (showHit) passenger->bounce(-passenger->fallSpeed(), context.play.data.sprites().bouncedPassenger);
    else passenger->bounceUnseen(-passenger->fallSpeed());
    return true;
}

void Enemy::scoreStun(units::Int16 score, const EnemyContext& context) const {
    context.play.session.addScore(static_cast<uint32_t>(score.bits()));
    context.play.report({events::EventKind::EnemyStunned, std::nullopt, index_, score.value()});
}

}  // namespace ugh::enemies
