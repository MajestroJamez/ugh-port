#include "passengers/route/Boarding.hpp"

#include "passengers/route/Impatient.hpp"
#include "passengers/route/Riding.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Boarding Boarding::instance{};

/** The bubble goes (also on the water: SwimBoarding). */
void Boarding::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.hideBubble();
}

void Boarding::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    std::optional<int> copter = context.play.level.copters().landedOn(passenger.route().pickupPad());
    if (!copter) {
        passenger.changeState(Impatient::instance, context);
        return;
    }
    if (passenger.walkTowards(context.play.level.copters()[*copter])) Riding::board(passenger, *copter, context);
}

}  // namespace ugh::passengers::route
