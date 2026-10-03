#include "passengers/route/Boarding.hpp"

#include "passengers/route/Impatient.hpp"
#include "passengers/route/Riding.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Boarding Boarding::instance{};

/** The bubble goes (also on the water: SwimBoarding). */
void Boarding::enter(RoutePassenger& passenger, const PassengerContext&) const { passenger.startBoarding(); }

void Boarding::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    world::copter::Copter* copter = context.level.copters().landedOn(passenger.route().pickupPad());
    if (!copter) {
        passenger.changeState(Impatient::instance, context);
        return;
    }
    if (passenger.walkTowards(*copter)) Riding::board(passenger, *copter, context);
}

}  // namespace ugh::passengers::route
