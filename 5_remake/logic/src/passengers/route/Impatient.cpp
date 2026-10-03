#include "passengers/route/Impatient.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Waiting.hpp"

namespace ugh::passengers::route {

const Impatient Impatient::instance{};

/** The impatient bubble (also on the water: SwimWaving). */
void Impatient::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.startWaving(context.data.sprites());
}

void Impatient::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.wave();
    if (!passenger.pickupWait().tick()) return;
    if (context.level.copters().landedOnWithRoom(passenger.route().pickupPad())) {
        passenger.changeState(Calling::instance, context);
    } else {
        passenger.changeState(Waiting::instance, context);
    }
}

}  // namespace ugh::passengers::route
