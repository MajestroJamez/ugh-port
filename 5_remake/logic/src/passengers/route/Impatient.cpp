#include "passengers/route/Impatient.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Waiting.hpp"

namespace ugh::passengers::route {

const Impatient Impatient::instance{};

/** The impatient bubble (also on the water: SwimWaving). */
void Impatient::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.restartAnimation();
    passenger.showBubble(context.data.sprites().impatientBubble);
    passenger.call().start(WAVE_TIME);
}

void Impatient::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (!passenger.call().over()) return;
    if (context.level.copters().emptyLandedOn(passenger.route().pickupPad(context.level))) {
        passenger.changeState(Calling::instance, context);
    } else {
        passenger.changeState(Waiting::instance, context);
    }
}

}  // namespace ugh::passengers::route
