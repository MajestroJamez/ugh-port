#include "passengers/walking/Riding.hpp"

#include "passengers/walking/WalkingAway.hpp"

namespace ugh::passengers {

const Riding Riding::instance{};

void Riding::board(model::Passenger& passenger, int copter, model::Level& level) {
    passenger.counter().setCarrier(copter);
    passenger.changeState(instance, level);
}

/**
 * 113b:19fb - gets in: the copter gets the fare and shows the number of the target pad. A swimmer pays the fare of
 * its swimming kind and gets its walking kind back first (19e0), with that kind's look.
 */
void Riding::enter(model::Passenger& passenger, model::Level& level) const {
    int player = passenger.counter().carrier();
    core::Word fare = passenger.kind().fare, fareMin = passenger.kind().fareMin;
    if (passenger.kind().type == data::PassengerKind::Type::Swimming) passenger.switchKind();
    level.copter(player).takeOnBoard(passenger.kind().look, level.pad(passenger.targetPad()).number(), fare, fareMin);
    passenger.hide();
    passenger.startQuickDeliveryTime();
    level.pad(passenger.pickupPad()).vacate();
    level.report({core::EventKind::PassengerBoarded, player, passenger.index()});
}

/** 113b:1a42 - the fare drops to its minimum, the bonus time runs out, until the copter lands at the target. */
void Riding::update(model::Passenger& passenger, model::Level& level) const {
    model::Copter& copter = level.copter(passenger.counter().carrier());
    copter.lowerFare();
    passenger.tickQuickDeliveryTime();
    if (copter.landedOn(passenger.targetPad())) passenger.changeState(WalkingAway::instance, level);
}

}  // namespace ugh::passengers
