#include "passengers/standing/Falling.hpp"

#include "passengers/Gone.hpp"
#include "passengers/standing/StartStanding.hpp"

namespace ugh::passengers {

namespace {

using core::Fixed;
using core::Word;

constexpr data::Sprite DROPPED_SPRITE = 0x221;
constexpr Word GRAVITY = 2;   // 1/32 px per frame, per frame
// where it falls from: below the copter's top left corner, in pixels
constexpr Word DROP_X = 16, DROP_Y = 10;
// it is gone past these (its top left corner)
constexpr Fixed LEFT_EDGE(-0x200), RIGHT_EDGE(0x2800), BOTTOM_EDGE(0x1800);

}  // namespace

const Falling Falling::instance{};

/** 113b:1c81 - let go: falls from under the copter, with the copter's speed. */
void Falling::enter(model::Passenger& passenger, model::Level& level) const {
    int player = passenger.counter().carrier();
    model::Copter& copter = level.copter(player);
    const data::Box& box = passenger.kind().box;
    copter.unload();
    // the copter's speeds in 1/64 Fixed become twice as much in Fixed per frame (SAR 5)
    passenger.timer().setDropSpeed(Fixed(copter.speedX().raw() >> 5));
    passenger.setFallSpeed(copter.speedY().raw() >> 5);
    passenger.moveTo(copter.x() + Fixed::fromPixels(DROP_X) - Fixed::fromPixels(box.x),
                     copter.y() + Fixed::fromPixels(DROP_Y) - Fixed(box.y << 4));   // half its height
    passenger.showSprite(DROPPED_SPRITE);
    level.report({core::EventKind::PassengerDropped, player, passenger.index()});
}

/** 113b:1cee - falls; lands on a pad it comes down onto, or is gone off the screen. */
void Falling::update(model::Passenger& passenger, model::Level& level) const {
    const data::Box& box = passenger.kind().box;
    Fixed x = passenger.x() + passenger.timer().dropSpeed();
    if (x <= LEFT_EDGE || x >= RIGHT_EDGE) {
        passenger.changeState(Gone::instance, level);
        return;
    }
    passenger.moveToX(x);
    Word speed = passenger.fallSpeed() + GRAVITY;
    passenger.setFallSpeed(speed);
    if (speed < 0) {   // still on its way up
        passenger.moveToY(passenger.y() + Fixed(speed));
        return;
    }
    Fixed before = passenger.y();
    Fixed y = before + Fixed(speed);
    if (y >= BOTTOM_EDGE) {
        passenger.changeState(Gone::instance, level);
        return;
    }
    passenger.moveToY(y);
    Word feet = y.pixels() + box.y, feetBefore = before.pixels() + box.y;
    Word middle = passenger.x().pixels() + box.x;
    for (int i = 0; i < level.padCount(); i++) {
        const model::Pad& pad = level.pad(i);
        if (feetBefore <= pad.y() && feet >= pad.y() && pad.spans(middle)) {
            passenger.moveToY(Fixed::fromPixels(pad.y() - box.y));
            passenger.continueIn(StartStanding::instance, level);
            return;
        }
    }
}

}  // namespace ugh::passengers
