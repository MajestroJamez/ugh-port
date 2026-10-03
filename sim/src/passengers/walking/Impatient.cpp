#include "passengers/walking/Impatient.hpp"

#include "passengers/walking/Calling.hpp"
#include "passengers/walking/Waiting.hpp"

namespace ugh::passengers {

namespace {

constexpr data::Sprite IMPATIENT_BUBBLE = 0x112;
constexpr core::Word WAVE_TIME = 0x8c;

}  // namespace

const Impatient Impatient::instance{};

/** 113b:17e6 - the impatient bubble (also on the water: SwimWaving). */
void Impatient::enter(model::Passenger& passenger, model::Level&) const {
    passenger.restartAnimation();
    passenger.showBubble(IMPATIENT_BUBBLE);
    passenger.counter().startCountdown(WAVE_TIME);
}

/** 113b:180a - waves for a while, then calls the next copter with room, or waits again. */
void Impatient::update(model::Passenger& passenger, model::Level& level) const {
    if (fellIntoWater(passenger, level)) return;
    if (knockedIntoWater(passenger, level)) return;
    if (passenger.animate()) passenger.show(*passenger.kind().waving);
    if (!passenger.counter().tick()) return;
    if (level.emptyCopterLandedOn(passenger.pickupPad())) passenger.changeState(Calling::instance, level);
    else passenger.changeState(Waiting::instance, level);
}

}  // namespace ugh::passengers
