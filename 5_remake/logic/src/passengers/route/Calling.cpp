#include "passengers/route/Calling.hpp"

#include "passengers/route/Boarding.hpp"
#include "passengers/route/Impatient.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

const Calling Calling::instance{};

/** The bubble shows the target pad (also on the water: SwimCalling). */
void Calling::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    const data::SpriteIds& sprites = context.play.data.sprites();
    passenger.restartAnimation();
    int bubble = sprites.firstDestinationBubble + passenger.route().targetPad();
    if (bubble > sprites.lastDestinationBubble) bubble = sprites.lastDestinationBubble;
    passenger.showBubble(bubble);
    passenger.call().start(CALL_TIME);
}

/** It waves impatiently when the copter left or is full. */
void Calling::stay(RoutePassenger& passenger, const PassengerContext& context) const {
    std::optional<int> copter = context.play.level.copters().landedOn(passenger.route().pickupPad());
    if (!copter || !context.play.level.copters()[*copter].cabin().hasRoom()) {
        passenger.changeState(Impatient::instance, context);
        return;
    }
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.call().over()) passenger.changeState(Boarding::instance, context);
}

}  // namespace ugh::passengers::route
