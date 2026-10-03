#include "passengers/route/Impatient.hpp"

#include "passengers/route/Calling.hpp"
#include "passengers/route/OnPickupPad.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Waiting.hpp"

namespace ugh::passengers::route {

const Impatient Impatient::instance{};

/** The impatient bubble (also on the water: SwimWaving). */
void Impatient::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    passenger.restartAnimation();
    passenger.showBubble(context.play.data.sprites().impatientBubble);
    passenger.startCallTime(WAVE_TIME);
}

void Impatient::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (OnPickupPad::fellIntoWater(passenger, context)) return;
    if (OnPickupPad::knockedIntoWater(passenger, context)) return;
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (!passenger.callTimeOver()) return;
    if (context.play.level.emptyCopterLandedOn(passenger.pickupPad())) passenger.changeState(Calling::instance, context);
    else passenger.changeState(Waiting::instance, context);
}

}  // namespace ugh::passengers::route
