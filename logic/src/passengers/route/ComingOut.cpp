#include "passengers/route/ComingOut.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/Waiting.hpp"

namespace ugh::passengers::route {

const ComingOut ComingOut::instance{};

void ComingOut::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Pad& pad = context.play.level.pad(passenger.pickupPad());
    pad.occupy(passenger.index());
    passenger.standAtDoor(pad.place());
    passenger.restartAnimation();
}

void ComingOut::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (!passenger.animate()) return;
    const data::Animation& door = *passenger.kind().comingOut;
    if (passenger.pastEndOf(door)) {
        passenger.changeState(Waiting::instance, context);
        return;
    }
    passenger.showFrameOf(door);
}

}  // namespace ugh::passengers::route
