#include "passengers/route/Waiting.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Waiting Waiting::instance{};

void Waiting::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.pickupWait().startWaiting();
    passenger.hideBubble();
}

/** A step per animation frame to the waiting spot of the pad; it stands there. */
void Waiting::walk(RoutePassenger& passenger, const PassengerContext&) const {
    if (!passenger.animate()) return;
    int spot = passenger.route().pickupPad().place().wait;
    if (!passenger.stepTowards(spot)) {
        passenger.pickupWait().walkToSpot();
        return;
    }
    if (passenger.pickupWait().reachSpot()) passenger.rewindAnimation();   // it just got there
    passenger.show(*passenger.kind().standing);
}

void Waiting::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    if (context.level.copters().landedOnWithRoom(passenger.route().pickupPad()))
        passenger.changeState(Calling::instance, context);
}

}  // namespace ugh::passengers::route
