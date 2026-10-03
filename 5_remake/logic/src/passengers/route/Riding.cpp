#include "passengers/route/Riding.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/WalkingToDoor.hpp"

namespace ugh::passengers::route {

namespace {

constexpr int QUICK_DELIVERY_TIME = 200;   // frames of riding that still earn a bonus item

}  // namespace

const Riding Riding::instance{};

void Riding::board(RoutePassenger& passenger, world::copter::Copter& copter, const PassengerContext& context) {
    passenger.ride().start(copter, QUICK_DELIVERY_TIME);
    passenger.changeState(instance, context);
}

/**
 * It gets in: the copter shows the number of its target pad and gets the fare. A swimmer pays the fare of its water
 * kind and gets its own kind back. The pickup pad is free again - even after a rescue from the water, when another
 * passenger may wait there by now.
 */
void Riding::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    world::copter::Copter& copter = *passenger.ride().carrier();
    // the kind it has now pays: a rescued swimmer its water kind, so it is taken before the swimmer is itself again
    const data::kinds::AnimatedPassengerKind& paying = passenger.kind();
    passenger.form().outOfWater();
    int fare = paying.fare, fareMin = paying.fareMin;
    int destination = passenger.route().targetPad().place().number;
    copter.cabin().takeOnBoard(passenger.form().land().look, destination, fare, fareMin);
    passenger.hide();
    passenger.route().pickupPad().vacate();
    context.report({events::EventKind::PassengerBoarded, copter.player(), passenger.index()});
}

void Riding::update(RoutePassenger& passenger, const PassengerContext& context) const {
    world::copter::Copter& copter = *passenger.ride().carrier();
    copter.cabin().lowerFare();
    passenger.ride().tick();
    if (copter.landedOn(passenger.route().targetPad())) passenger.changeState(WalkingToDoor::instance, context);
}

}  // namespace ugh::passengers::route
