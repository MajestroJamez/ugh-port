#include "passengers/route/BehindDoor.hpp"

#include "passengers/route/ComingOut.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const BehindDoor BehindDoor::instance{};

void BehindDoor::update(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.hide();
    if (!passenger.route().arrivalDue()) return;
    if (!context.play.level.pad(passenger.route().pickupPad()).free()) return;
    passenger.changeState(ComingOut::instance, context);
}

}  // namespace ugh::passengers::route
