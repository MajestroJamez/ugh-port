#include "passengers/route/ComingOut.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Waiting.hpp"

namespace ugh::passengers::route {

const ComingOut ComingOut::instance{};

void ComingOut::enter(RoutePassenger& passenger, const PassengerContext&) const {
    world::scenery::Pad& pad = passenger.route().pickupPad();
    pad.occupy(passenger);
    passenger.standAtDoor(pad.place());
    passenger.restartAnimation();
}

void ComingOut::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (!passenger.animate()) return;
    const data::kinds::Animation& door = *passenger.form().land().comingOut;
    if (passenger.pastEndOf(door)) {
        passenger.changeState(Waiting::instance, context);
        return;
    }
    passenger.showFrameOf(door);
}

}  // namespace ugh::passengers::route
