#include "passengers/route/Waiting.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Waiting Waiting::instance{};

void Waiting::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.call().startWaiting();
    passenger.hideBubble();
}

/** A step per animation frame to the waiting spot of the pad; it stands there. */
void Waiting::walk(RoutePassenger& passenger, const PassengerContext& context) const {
    if (!passenger.animate()) return;
    int spot = passenger.route().pickupPad(context.level).place().wait;
    if (!passenger.stepTowards(spot)) {
        passenger.call().walkToSpot();
        return;
    }
    if (passenger.call().reachSpot()) passenger.rewindAnimation();   // it just got there
    passenger.show(*passenger.kind().standing);
}

void Waiting::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    if (context.level.copters().emptyLandedOn(passenger.route().pickupPad(context.level)))
        passenger.changeState(Calling::instance, context);
}

}  // namespace ugh::passengers::route
