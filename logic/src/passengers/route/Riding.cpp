#include "passengers/route/Riding.hpp"

#include "passengers/route/RoutePassenger.hpp"
#include "passengers/route/WalkingToDoor.hpp"

namespace ugh::passengers::route {

const Riding Riding::instance{};

void Riding::board(RoutePassenger& passenger, int player, const PassengerContext& context) {
    passenger.setCarrier(player);
    passenger.changeState(instance, context);
}

/**
 * It gets in: the copter shows the number of its target pad and gets the fare. A swimmer pays the fare of its water
 * kind and gets its own kind back. The pickup pad is free again - even after a rescue from the water, when another
 * passenger may wait there by now.
 */
void Riding::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Level& level = context.play.level;
    int player = *passenger.carrier();
    units::Int16 fare = passenger.kind().fare, fareMin = passenger.kind().fareMin;
    if (passenger.kind().type == data::PassengerKind::Type::Water) passenger.switchKind();
    level.copter(player).takeOnBoard(passenger.kind().look, level.pad(passenger.targetPad()).place().number, fare, fareMin);
    passenger.hide();
    passenger.startQuickDeliveryTime(QUICK_DELIVERY_TIME);
    level.pad(passenger.pickupPad()).vacate();
    context.play.report({events::EventKind::PassengerBoarded, player, passenger.index()});
}

void Riding::update(RoutePassenger& passenger, const PassengerContext& context) const {
    world::Copter& copter = context.play.level.copter(*passenger.carrier());
    copter.lowerFare();
    passenger.tickQuickDeliveryTime();
    if (copter.landedOn(passenger.targetPad())) passenger.changeState(WalkingToDoor::instance, context);
}

}  // namespace ugh::passengers::route
