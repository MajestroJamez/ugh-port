#include "passengers/standing/Hanging.hpp"

#include "passengers/standing/Falling.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::passengers::standing {

const Hanging Hanging::instance{};

void Hanging::enter(StandingPassenger& passenger, const PassengerContext& context) const {
    int player = *passenger.carrier();
    context.play.level.copter(player).pickUpHanging(passenger.kind().look);
    passenger.hide();
    context.play.report({events::EventKind::PassengerBoarded, player, passenger.index()});
}

void Hanging::update(StandingPassenger& passenger, const PassengerContext& context) const {
    if (context.play.level.copter(*passenger.carrier()).controls().fire) passenger.changeState(Falling::instance, context);
}

}  // namespace ugh::passengers::standing
