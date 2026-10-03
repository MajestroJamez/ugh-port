#include "passengers/standing/Standing.hpp"

#include "passengers/standing/Hanging.hpp"
#include "passengers/standing/StandingPassenger.hpp"
#include "physics/TouchBox.hpp"

namespace ugh::passengers::standing {

const Standing Standing::instance{};

void Standing::update(StandingPassenger& passenger, const PassengerContext& context) const {
    std::optional<int> copter =
        physics::TouchBox(passenger.kind().box, passenger.x(), passenger.y()).firstCopterIn(context.play.level);
    if (copter && context.play.level.copter(*copter).hasRoom()) {
        passenger.setCarrier(*copter);
        passenger.changeState(Hanging::instance, context);
        return;
    }
    passenger.showSprite(context.play.data.sprites().standingPassenger);
}

}  // namespace ugh::passengers::standing
