#include "physics/Ballistics.hpp"

#include "world/Screen.hpp"

namespace ugh::physics {

using units::Fixed;
using units::Int16;

Ballistics::Result Ballistics::fall(Body& body, const world::Level& level) const {
    Fixed x = body.x + body.speedX;
    if (world::Screen::pastSide(x)) return Result::Gone;
    body.x = x;
    body.fallSpeed += gravity_;
    if (body.fallSpeed < 0) {   // still on its way up
        body.y += Fixed::fromRaw(body.fallSpeed);
        return Result::Flying;
    }
    Fixed before = body.y;
    Fixed y = before + Fixed::fromRaw(body.fallSpeed);
    if (y >= world::Screen::BOTTOM) return Result::Gone;
    body.y = y;
    Int16 bottom = y.pixels() + body.anchorY, bottomBefore = before.pixels() + body.anchorY;
    Int16 middle = body.x.pixels() + body.anchorX;
    for (int i = 0; i < level.padCount(); i++) {
        const data::PadDefinition& pad = level.pad(i).place();
        if (landsOn(pad, bottomBefore, bottom, middle)) {
            body.y = Fixed::fromPixels(pad.y - body.anchorY);
            return Result::Landed;
        }
    }
    return Result::Flying;
}

bool Ballistics::landsOn(const data::PadDefinition& pad, Int16 bottomBefore, Int16 bottom, Int16 middle) const {
    if (landing_ == Landing::Passenger) return bottomBefore <= pad.y && bottom >= pad.y && pad.spans(middle);
    return bottomBefore < pad.y && bottom >= pad.y && middle >= pad.left && middle - 1 <= pad.right;
}

}  // namespace ugh::physics
