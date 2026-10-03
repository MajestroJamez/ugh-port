#include "enemies/Enemy.hpp"

#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::enemies {

bool Enemy::bounceFallingPassenger(const EnemyContext& context) const {
    passengers::standing::StandingPassenger* passenger = context.passengers.fallingOnto(x(), y());
    if (!passenger) return false;
    passenger->bounce(-passenger->fallSpeed(), context.data.sprites().bouncedPassenger);
    return true;
}

bool Enemy::bounceFallingPassengerUnseen(const EnemyContext& context) const {
    passengers::standing::StandingPassenger* passenger = context.passengers.fallingOnto(x(), y());
    if (!passenger) return false;
    passenger->bounceUnseen(-passenger->fallSpeed());
    return true;
}

void Enemy::scoreStun(int score, const EnemyContext& context) const {
    context.session.score().add(static_cast<uint32_t>(score));
    context.report({events::EventKind::EnemyStunned, std::nullopt, index(), score});
}

}  // namespace ugh::enemies
