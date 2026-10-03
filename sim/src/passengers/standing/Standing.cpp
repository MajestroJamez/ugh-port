#include "passengers/standing/Standing.hpp"

#include "passengers/standing/Hanging.hpp"

namespace ugh::passengers {

namespace {

constexpr data::Sprite STANDING_SPRITE = 0x220;

}  // namespace

const Standing Standing::instance{};

/** 113b:1c0f */
void Standing::enter(model::Passenger& passenger, model::Level&) const { passenger.restartAnimation(); }

/** 113b:1c27 - waits for a copter with room to touch it. */
void Standing::update(model::Passenger& passenger, model::Level& level) const {
    int copter = level.copterTouching(passenger.kind().box, passenger.x(), passenger.y());
    if (copter != model::Level::NONE && level.copter(copter).hasRoom()) {
        passenger.counter().setCarrier(copter);
        passenger.changeState(Hanging::instance, level);
        return;
    }
    passenger.showSprite(STANDING_SPRITE);
}

}  // namespace ugh::passengers
