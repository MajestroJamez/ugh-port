#include "passengers/route/Waiting.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/OnPickupPad.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Waiting Waiting::instance{};

void Waiting::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.call().startWaiting();
    passenger.hideBubble();
}

void Waiting::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (OnPickupPad::fellIntoWater(passenger, context)) return;
    if (passenger.animate()) {
        const data::PassengerKind& kind = passenger.kind();
        units::Int16 spot = context.play.level.pad(passenger.route().pickupPad()).place().wait;
        units::Int16 feet = passenger.feetX();
        if (feet == spot) {
            if (passenger.call().reachSpot()) passenger.rewindAnimation();   // it just got there
            passenger.show(*kind.standing);
        } else {
            passenger.call().walkToSpot();
            bool right = feet < spot;
            passenger.show(kind.walking.towards(right));
            passenger.stepBy(right ? 1 : -1);
        }
    }
    if (OnPickupPad::knockedIntoWater(passenger, context)) return;
    if (context.play.level.emptyCopterLandedOn(passenger.route().pickupPad()))
        passenger.changeState(Calling::instance, context);
}

}  // namespace ugh::passengers::route
