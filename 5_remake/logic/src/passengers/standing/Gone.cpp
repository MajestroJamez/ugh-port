#include "passengers/standing/Gone.hpp"

#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::passengers::standing {

const Gone Gone::instance{};

void Gone::enter(StandingPassenger& passenger, const PassengerContext&) const { passenger.hide(); }

}  // namespace ugh::passengers::standing
