#include "passengers/route/Calling.hpp"

#include "passengers/route/Boarding.hpp"
#include "passengers/route/Impatient.hpp"
#include "passengers/route/OnPickupPad.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Calling Calling::instance{};

/** The bubble shows the target pad (also on the water: SwimCalling). */
void Calling::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    const data::SpriteIds& sprites = context.play.data.sprites();
    passenger.restartAnimation();
    units::Int16 bubble = sprites.firstDestinationBubble + passenger.targetPad();
    // compared unsigned, as the original does
    if (units::Int16::unsignedLess(sprites.lastDestinationBubble, bubble)) bubble = sprites.lastDestinationBubble;
    passenger.showBubble(bubble.value());
    passenger.startCallTime(CALL_TIME);
}

/** It waves impatiently when the copter left or is full. */
void Calling::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (OnPickupPad::fellIntoWater(passenger, context)) return;
    if (OnPickupPad::knockedIntoWater(passenger, context)) return;
    std::optional<int> copter = context.play.level.copterLandedOn(passenger.pickupPad());
    if (!copter || !context.play.level.copter(*copter).hasRoom()) {
        passenger.changeState(Impatient::instance, context);
        return;
    }
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.callTimeOver()) passenger.changeState(Boarding::instance, context);
}

}  // namespace ugh::passengers::route
