// Every state of the bonus items.
#pragma once

#include <vector>

#include "bonuses/BonusState.hpp"

namespace ugh::bonuses {

/** The registry of the states (the replay projection finds them by name). */
class BonusStates {
public:
    static const std::vector<const BonusState*>& all();
};

}  // namespace ugh::bonuses
