#include "passengers/walking/Arriving.hpp"

#include "passengers/walking/Appearing.hpp"

namespace ugh::passengers {

const Arriving Arriving::instance{};

/** 113b:1509 */
void Arriving::update(model::Passenger& passenger, model::Level& level) const {
    passenger.hide();
    if (!passenger.timer().arrivalDue()) return;
    if (!level.pad(passenger.pickupPad()).free()) return;
    passenger.changeState(Appearing::instance, level);
}

}  // namespace ugh::passengers
