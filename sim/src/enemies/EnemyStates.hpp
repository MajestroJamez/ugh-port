// Every state of the enemies.
#pragma once

#include <vector>

#include "enemies/EnemyState.hpp"

namespace ugh::enemies {

/** The registry of the states (the replay projection finds them by name). */
class EnemyStates {
public:
    static const std::vector<const EnemyState*>& all();
};

}  // namespace ugh::enemies
