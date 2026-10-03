// Every state of the passengers.
#pragma once

#include <vector>

#include "passengers/PassengerState.hpp"

namespace ugh::passengers {

/** The registry of the states (the replay projection finds them by name). */
class PassengerStates {
public:
    static const std::vector<const PassengerState*>& all();
};

}  // namespace ugh::passengers
