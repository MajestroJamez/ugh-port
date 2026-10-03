#include "passengers/route/NextStop.hpp"

#include "passengers/route/BehindDoor.hpp"
#include "passengers/route/Gone.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const NextStop NextStop::instance{};

void NextStop::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (passenger.route().finished()) {
        context.level.passengerFinished(context.events);
        passenger.continueIn(Gone::instance, context);
        return;
    }
    passenger.route().startArrival();
    passenger.changeState(BehindDoor::instance, context);
}

}  // namespace ugh::passengers::route
