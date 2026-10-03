// What the states of a walker share.
#pragma once

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** The states of a walker that a falling passenger can stun (Walking, Watching, Charging, Recovering). */
class WalkerState : public EnemyState {
protected:
    /** 113b:28ee - a falling passenger bounced off it: the walker is stunned. */
    static bool stunnedByPassenger(model::Enemy& enemy, model::Level& level);
};

}  // namespace ugh::enemies
