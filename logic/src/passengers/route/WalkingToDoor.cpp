#include "passengers/route/WalkingToDoor.hpp"

#include "passengers/route/GoingIn.hpp"
#include "passengers/route/RoutePassenger.hpp"

namespace ugh::passengers::route {

namespace {

using units::Fixed;

constexpr units::Int16 COPTER_DOOR = 16;   // from the copter's left edge to where the passenger gets out
// where a quick delivery's bonus item drops: from the copter's top left corner, in pixels
constexpr units::Int16 BONUS_DROP_X = 16, BONUS_DROP_Y = 10;

}  // namespace

const WalkingToDoor WalkingToDoor::instance{};

/** It pays the fare times the score multiplier; a quick delivery drops a bonus item that raises the multiplier. */
void WalkingToDoor::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    int player = *passenger.carrier();
    world::Copter& copter = context.play.level.copter(player);
    world::Session& session = context.play.session;
    copter.unload();
    passenger.moveToX(copter.x() + Fixed::fromPixels(COPTER_DOOR - passenger.kind().box.x));
    uint32_t points = static_cast<uint32_t>(copter.fare().bits()) * static_cast<uint32_t>(session.multiplier());
    session.addScore(points);
    context.play.report({events::EventKind::PassengerPaid, player, passenger.index(), static_cast<int>(points)});
    if (passenger.deliveredQuickly() && session.multiplierBelowLimit()) {
        context.bonuses.drop(context.play.data.rules().quickDeliveryBonus(), copter.x() + Fixed::fromPixels(BONUS_DROP_X),
                             copter.y() + Fixed::fromPixels(BONUS_DROP_Y), Fixed(), 0, context.play.diagnostics);
        context.play.report({events::EventKind::QuickDelivery, player, passenger.index()});
    }
    const data::PadDefinition& target = context.play.level.pad(passenger.targetPad()).place();
    passenger.moveToY(Fixed::fromPixels(target.y - passenger.kind().box.y));
    passenger.restartAnimation();
}

/**
 * A step per animation frame towards the door. The first step decides from where the passenger was seen at boarding:
 * it was hidden in the copter since.
 */
void WalkingToDoor::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (!passenger.animate()) return;
    units::Int16 door = context.play.level.pad(passenger.targetPad()).place().door;
    units::Int16 feet = passenger.feetX();
    if (feet == door) {
        passenger.changeState(GoingIn::instance, context);
        return;
    }
    bool right = feet < door;
    passenger.show(passenger.kind().walking.towards(right));
    passenger.stepBy(right ? 1 : -1);
    // the original looks at the position it moved to as well
    if (passenger.x().pixels() + passenger.kind().box.x == door) passenger.changeState(GoingIn::instance, context);
}

}  // namespace ugh::passengers::route
