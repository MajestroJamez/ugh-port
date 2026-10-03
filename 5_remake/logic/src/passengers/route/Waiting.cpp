#include "passengers/route/Waiting.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/OnPickupPad.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Waiting Waiting::instance{};

void Waiting::enter(RoutePassenger& passenger, const PassengerContext&) const {
    passenger.restartAnimation();
    passenger.setWaitingSpot(WaitingSpot::Starting);
    passenger.hideBubble();
}

void Waiting::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (OnPickupPad::fellIntoWater(passenger, context)) return;
    if (passenger.animate()) {
        const data::PassengerKind& kind = passenger.kind();
        units::Int16 spot = context.play.level.pad(passenger.pickupPad()).place().wait;
        units::Int16 feet = passenger.feetX();
        if (feet == spot) {
            if (passenger.waitingSpot() != WaitingSpot::Reached) passenger.rewindAnimation();   // it just got there
            passenger.show(*kind.standing);
            passenger.setWaitingSpot(WaitingSpot::Reached);
        } else {
            passenger.setWaitingSpot(WaitingSpot::Walking);
            bool right = feet < spot;
            passenger.show(kind.walking.towards(right));
            passenger.stepBy(right ? 1 : -1);
        }
    }
    if (OnPickupPad::knockedIntoWater(passenger, context)) return;
    if (context.play.level.emptyCopterLandedOn(passenger.pickupPad())) passenger.changeState(Calling::instance, context);
}

}  // namespace ugh::passengers::route
