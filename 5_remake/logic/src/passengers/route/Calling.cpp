#include "passengers/route/Calling.hpp"

#include "passengers/route/Boarding.hpp"
#include "passengers/route/Impatient.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Calling Calling::instance{};

/** The bubble shows the target pad (also on the water: SwimCalling). */
void Calling::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.startCalling(context.data.sprites());
}

/** It waves impatiently when the copter left or is full. */
void Calling::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    const world::copter::Copter* copter = context.level.copters().landedOn(passenger.route().pickupPad());
    if (!copter || !copter->cabin().hasRoom()) {
        passenger.changeState(Impatient::instance, context);
        return;
    }
    passenger.wave();
    if (passenger.pickupWait().tick()) passenger.changeState(Boarding::instance, context);
}

}  // namespace ugh::passengers::route
