#include "passengers/route/Gone.hpp"

#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Gone Gone::instance{};

void Gone::enter(RoutePassenger& passenger, const PassengerContext&) const { passenger.hide(); }

}  // namespace ugh::passengers::route
