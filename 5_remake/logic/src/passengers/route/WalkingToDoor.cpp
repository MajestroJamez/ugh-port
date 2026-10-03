#include "passengers/route/WalkingToDoor.hpp"

#include "passengers/route/GoingIn.hpp"
#include "passengers/route/RoutePassenger.hpp"
#include "world/copter/CopterShape.hpp"

namespace ugh::passengers::route {

namespace {

using units::Fixed;
using world::copter::CopterShape;

}  // namespace

const WalkingToDoor WalkingToDoor::instance{};

/** It pays the fare times the score multiplier; a quick delivery drops a bonus item that raises the multiplier. */
void WalkingToDoor::enter(RoutePassenger& passenger, const PassengerContext& context) const {
    world::copter::Copter& copter = *passenger.ride().carrier();
    world::session::Session& session = context.session;
    copter.cabin().unload();
    passenger.moveToX(copter.motion().x() + Fixed::fromPixels(CopterShape::DOOR_X - passenger.kind().box.x));
    uint32_t points = static_cast<uint32_t>(copter.cabin().fare()) * static_cast<uint32_t>(session.score().multiplier());
    session.score().add(points);
    context.report({events::EventKind::PassengerPaid, copter.player(), passenger.index(), static_cast<int>(points)});
    if (passenger.ride().quick() && session.score().multiplierBelowLimit()) {
        context.bonuses.drop(*context.data.rules().quickDeliveryBonus, copter.dropX(), copter.dropY(), Fixed(), Fixed(),
                             context.diagnostics);
        context.report({events::EventKind::QuickDelivery, copter.player(), passenger.index()});
    }
    const data::levels::PadDefinition& target = passenger.route().targetPad().place();
    passenger.moveToY(Fixed::fromPixels(target.y - passenger.kind().box.y));
    passenger.restartAnimation();
}

/**
 * A step per animation frame towards the door. The first step decides from where the passenger was seen at boarding:
 * it was hidden in the copter since.
 */
void WalkingToDoor::update(RoutePassenger& passenger, const PassengerContext& context) const {
    if (!passenger.animate()) return;
    int door = passenger.route().targetPad().place().door;
    if (passenger.stepTowards(door)) {
        passenger.changeState(GoingIn::instance, context);
        return;
    }
    // the original looks at the position it moved to as well
    if (passenger.x().pixels() + passenger.kind().box.x == door) passenger.changeState(GoingIn::instance, context);
}

}  // namespace ugh::passengers::route
