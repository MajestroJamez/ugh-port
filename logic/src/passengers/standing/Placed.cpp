#include "passengers/standing/Placed.hpp"

#include "passengers/standing/Standing.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::passengers::standing {

const Placed Placed::instance{};

void Placed::update(StandingPassenger& passenger, const PassengerContext& context) const {
    passenger.changeState(Standing::instance, context);
}

}  // namespace ugh::passengers::standing
