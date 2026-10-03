#include "passengers/standing/Standing.hpp"

#include "passengers/standing/Hanging.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::passengers::standing {

const Standing Standing::instance{};

void Standing::update(StandingPassenger& passenger, const PassengerContext& context) const {
    world::Copter* copter =
        physics::TouchBox(passenger.kind().box, passenger.x(), passenger.y()).firstCopterIn(context.level.copters());
    if (copter && copter->cabin().hasRoom()) {
        passenger.hangBelow(*copter);
        passenger.changeState(Hanging::instance, context);
        return;
    }
    passenger.showSprite(context.data.sprites().standingPassenger);
}

}  // namespace ugh::passengers::standing
