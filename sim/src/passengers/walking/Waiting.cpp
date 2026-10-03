#include "passengers/walking/Waiting.hpp"

#include "passengers/walking/Calling.hpp"

namespace ugh::passengers {

const Waiting Waiting::instance{};

/** 113b:15b4 */
void Waiting::enter(model::Passenger& passenger, model::Level&) const {
    passenger.restartAnimation();
    passenger.counter().startWaiting();
    passenger.hideBubble();
}

/** 113b:15d7 - walks to the waiting spot of the pad, stands there, and calls a copter that lands with room. */
void Waiting::update(model::Passenger& passenger, model::Level& level) const {
    if (fellIntoWater(passenger, level)) return;
    if (passenger.animate()) {
        const data::PassengerKind& kind = passenger.kind();
        core::Word spot = level.pad(passenger.pickupPad()).waitX();
        core::Word feet = passenger.feetX();
        if (feet == spot) {
            if (!passenger.counter().isAtSpot()) passenger.rewindAnimation();   // just arrived
            passenger.show(*kind.standing);
            passenger.counter().atSpot();
        } else {
            passenger.counter().walkingToSpot();
            bool right = feet < spot;
            passenger.show(kind.walking.towards(right));
            passenger.stepBy(right ? 1 : -1);
        }
    }
    if (knockedIntoWater(passenger, level)) return;
    if (level.emptyCopterLandedOn(passenger.pickupPad())) passenger.changeState(Calling::instance, level);
}

}  // namespace ugh::passengers
