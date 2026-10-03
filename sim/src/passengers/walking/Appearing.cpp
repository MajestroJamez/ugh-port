#include "passengers/walking/Appearing.hpp"

#include "passengers/walking/Waiting.hpp"

namespace ugh::passengers {

const Appearing Appearing::instance{};

/** 113b:153b - at the door of the pickup pad, which it takes. */
void Appearing::enter(model::Passenger& passenger, model::Level& level) const {
    model::Pad& pad = level.pad(passenger.pickupPad());
    pad.occupy(passenger.index());
    passenger.standAtDoor(pad.doorX(), pad.y());
    passenger.restartAnimation();
}

/** 113b:1582 - the door animation, then it waits. */
void Appearing::update(model::Passenger& passenger, model::Level& level) const {
    if (!passenger.animate()) return;
    const data::Animation& door = *passenger.kind().appearing;
    if (passenger.pastEndOf(door)) {
        passenger.changeState(Waiting::instance, level);
        return;
    }
    passenger.showFrameOf(door);
}

}  // namespace ugh::passengers
