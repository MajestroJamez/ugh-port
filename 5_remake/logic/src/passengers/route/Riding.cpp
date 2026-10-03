#include "passengers/route/Riding.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/WalkingToDoor.hpp"

namespace ugh::passengers::route {

const Riding Riding::instance{};

void Riding::board(RoutePassenger& passenger, world::Copter& copter, const PassengerContext& context) {
    passenger.ride().start(copter, QUICK_DELIVERY_TIME);
    passenger.changeState(instance, context);
}

/**
 * It gets in: the copter shows the number of its target pad and gets the fare. A swimmer pays the fare of its water
 * kind and gets its own kind back. The pickup pad is free again - even after a rescue from the water, when another
 * passenger may wait there by now.
 */
void Riding::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Copter& copter = *passenger.ride().carrier();
    int fare = passenger.kind().fare, fareMin = passenger.kind().fareMin;
    passenger.kinds().outOfWater();   // a rescued swimmer is itself again
    int destination = passenger.route().targetPad().place().number;
    copter.cabin().takeOnBoard(passenger.kinds().land().look, destination, fare, fareMin);
    passenger.hide();
    passenger.route().pickupPad().vacate();
    context.report({events::EventKind::PassengerBoarded, copter.player(), passenger.index()});
}

void Riding::update(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Copter& copter = *passenger.ride().carrier();
    copter.cabin().lowerFare();
    passenger.ride().tick();
    if (copter.landedOn(passenger.route().targetPad())) passenger.changeState(WalkingToDoor::instance, context);
}

}  // namespace ugh::passengers::route
