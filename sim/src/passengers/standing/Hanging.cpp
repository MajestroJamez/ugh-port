#include "passengers/standing/Hanging.hpp"

#include "passengers/standing/Falling.hpp"

namespace ugh::passengers {

const Hanging Hanging::instance{};

/** 113b:1c48 - picked up by the copter that touched it. */
void Hanging::enter(model::Passenger& passenger, model::Level& level) const {
    int player = passenger.counter().carrier();
    level.copter(player).pickUpHanging(passenger.kind().look);
    passenger.hide();
    level.report({core::EventKind::PassengerBoarded, player, passenger.index()});
}

/** 113b:1c6b */
void Hanging::update(model::Passenger& passenger, model::Level& level) const {
    if (level.copter(passenger.counter().carrier()).controls().fire) passenger.changeState(Falling::instance, level);
}

}  // namespace ugh::passengers
