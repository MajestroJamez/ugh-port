#include "enemies/Enemy.hpp"

#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::enemies {

passengers::standing::StandingPassenger* Enemy::bounceFallingPassenger(const EnemyContext& context,
                                                                     Rebound rebound) const {
    passengers::standing::StandingPassenger* passenger = context.passengers.fallingOnto(x(), y());
    if (!passenger) return nullptr;
    units::Fixed back = -passenger->fallSpeed();
    passenger->bounce(rebound == Rebound::Half ? back.half() : back, context.data.sprites().bouncedPassenger);
    return passenger;
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
