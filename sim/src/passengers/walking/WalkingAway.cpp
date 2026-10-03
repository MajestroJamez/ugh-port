#include "passengers/walking/WalkingAway.hpp"

#include "bonuses/Falling.hpp"
#include "passengers/walking/Entering.hpp"

namespace ugh::passengers {

namespace {

using core::Fixed;

constexpr core::Word COPTER_DOOR = 0x10;   // from the copter's left edge to where the passenger gets out
// where a quick delivery's bonus item drops: from the copter's top left corner, in pixels
constexpr core::Word BONUS_DROP_X = 16, BONUS_DROP_Y = 10;

}  // namespace

const WalkingAway WalkingAway::instance{};

/**
 * 113b:1a7e - delivered: gets out and pays the fare times the multiplier; a quick delivery drops a bonus item
 * that raises the multiplier.
 */
void WalkingAway::enter(model::Passenger& passenger, model::Level& level) const {
    int player = passenger.counter().carrier();
    model::Copter& copter = level.copter(player);
    model::GameSession& session = level.session();
    copter.unload();
    passenger.moveToX(copter.x() + Fixed::fromPixels(COPTER_DOOR - passenger.kind().box.x));
    // the original multiplies with the multiplier as a word, with the zero byte after it
    uint32_t points = static_cast<uint32_t>(copter.fare().bits()) * session.multiplier();
    if (points > 0xffff) core::audit::count("Q5 delivery points > 16 bits");
    session.addScore(points);
    level.report({core::EventKind::PassengerPaid, player, passenger.index(), static_cast<int>(points)});
    if (passenger.deliveredQuickly() && session.multiplierBelowLimit()) {
        bonuses::Falling::drop(level, level.data().quickDeliveryBonus(), copter.x() + Fixed::fromPixels(BONUS_DROP_X),
                               copter.y() + Fixed::fromPixels(BONUS_DROP_Y), Fixed(0), 0);
        level.report({core::EventKind::QuickDelivery, player, passenger.index()});
    }
    passenger.moveToY(Fixed::fromPixels(level.pad(passenger.targetPad()).y() - passenger.kind().box.y));
    passenger.restartAnimation();
}

/** 113b:1b29 - walks from the copter to the door of the target pad. */
void WalkingAway::update(model::Passenger& passenger, model::Level& level) const {
    if (!passenger.animate()) return;
    core::Word door = level.pad(passenger.targetPad()).doorX();
    core::Word feet = passenger.feetX();
    if (feet == door) {
        passenger.changeState(Entering::instance, level);
        return;
    }
    bool right = feet < door;
    passenger.show(passenger.kind().walking.towards(right));
    passenger.stepBy(right ? 1 : -1);
    // the original looks at the position it moved to as well
    if (passenger.x().pixels() + passenger.kind().box.x == door) passenger.changeState(Entering::instance, level);
}

}  // namespace ugh::passengers
