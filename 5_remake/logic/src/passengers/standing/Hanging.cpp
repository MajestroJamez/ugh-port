#include "passengers/standing/Hanging.hpp"

#include "passengers/standing/Falling.hpp"
#include "passengers/standing/StandingPassenger.hpp"

namespace ugh::passengers::standing {

const Hanging Hanging::instance{};

void Hanging::enter(StandingPassenger& passenger, const PassengerContext& context) const {
    world::copter::Copter& copter = *passenger.carrier();
    copter.cabin().pickUpHanging(passenger.kind().look);
    passenger.hide();
    context.report({events::EventKind::PassengerBoarded, copter.player(), passenger.index()});
}

void Hanging::update(StandingPassenger& passenger, const PassengerContext& context) const {
    if (passenger.carrier()->controls().fire) passenger.changeState(Falling::instance, context);
}

}  // namespace ugh::passengers::standing
