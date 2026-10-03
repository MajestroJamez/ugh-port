#include "passengers/walking/Calling.hpp"

#include "passengers/walking/Boarding.hpp"
#include "passengers/walking/Impatient.hpp"

namespace ugh::passengers {

namespace {

constexpr core::Word DESTINATION_BUBBLE = 0x10c;        // + the target pad: the bubble shows its number
constexpr core::Word LAST_DESTINATION_BUBBLE = 0x111;
constexpr core::Word CALL_TIME = 0x8c;

}  // namespace

const Calling Calling::instance{};

/** 113b:16f6 - shows the bubble with the destination (also on the water: SwimCalling). */
void Calling::enter(model::Passenger& passenger, model::Level&) const {
    passenger.restartAnimation();
    core::Word bubble = DESTINATION_BUBBLE + passenger.targetPad();
    if (core::Word::unsignedLess(LAST_DESTINATION_BUBBLE, bubble)) bubble = LAST_DESTINATION_BUBBLE;
    passenger.showBubble(bubble.bits());
    passenger.counter().startCountdown(CALL_TIME);
}

/** 113b:172a - calls the copter for a while, then walks to it; waves impatiently when it is gone or full. */
void Calling::update(model::Passenger& passenger, model::Level& level) const {
    if (fellIntoWater(passenger, level)) return;
    if (knockedIntoWater(passenger, level)) return;
    int copter = level.copterLandedOn(passenger.pickupPad());
    if (copter == model::Level::NONE || !level.copter(copter).hasRoom()) {
        passenger.changeState(Impatient::instance, level);
        return;
    }
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (passenger.counter().tick()) passenger.changeState(Boarding::instance, level);
}

}  // namespace ugh::passengers
