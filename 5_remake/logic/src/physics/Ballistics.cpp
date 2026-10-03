#include "physics/Ballistics.hpp"

#include "world/Screen.hpp"

namespace ugh::physics {

using units::Fixed;

Ballistics::Result Ballistics::fall(Body& body, const world::Level& level) const {
    Fixed x = body.x + body.speedX;
    if (world::Screen::pastSide(x)) return Result::Gone;
    body.x = x;
    body.fallSpeed += gravity_;
    if (body.fallSpeed < Fixed()) {   // still on its way up
        body.y += body.fallSpeed;
        return Result::Flying;
    }
    Fixed before = body.y;
    Fixed y = before + body.fallSpeed;
    if (y >= world::Screen::BOTTOM) return Result::Gone;
    body.y = y;
    int bottom = y.pixels() + body.anchorY, bottomBefore = before.pixels() + body.anchorY;
    int middle = body.x.pixels() + body.anchorX;
    for (const world::Pad& pad : level.pads()) {
        if (landsOn(pad.place(), bottomBefore, bottom, middle)) {
            body.y = Fixed::fromPixels(pad.place().y - body.anchorY);
            return Result::Landed;
        }
    }
    return Result::Flying;
}

bool Ballistics::landsOn(const data::levels::PadDefinition& pad, int bottomBefore, int bottom, int middle) const {
    if (landing_ == Landing::Passenger) return bottomBefore <= pad.y && bottom >= pad.y && pad.spans(middle);
    return bottomBefore < pad.y && bottom >= pad.y && middle >= pad.left && middle - 1 <= pad.right;
}

}  // namespace ugh::physics
