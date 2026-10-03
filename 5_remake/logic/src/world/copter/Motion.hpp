// Where a copter is and how fast it moves.
#pragma once

#include "units/Fixed.hpp"
#include "units/Speed.hpp"

namespace ugh::world::copter {

/**
 * Where a copter is and how fast it moves. The pixel position is what the collision probe sees: it follows x and y,
 * except that a copter thrown by a walker keeps its pixel row for a frame (`throwUp`).
 */
class Motion {
public:
    Motion() = default;
    /** At x, y with the pixel position the collision probe sees (it may lag behind y, see above), moving at vx, vy. */
    Motion(units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx, units::Speed vy)
        : x_(x), y_(y), pixelX_(pixelX), pixelY_(pixelY), vx_(vx), vy_(vy) {}

    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    int pixelX() const { return pixelX_; }
    int pixelY() const { return pixelY_; }
    units::Speed speedX() const { return vx_; }
    units::Speed speedY() const { return vy_; }

    void setSpeedX(units::Speed vx) { vx_ = vx; }
    void setSpeedY(units::Speed vy) { vy_ = vy; }
    /** No speed. */
    void stop() { vx_ = vy_ = units::Speed(); }
    /** To x, and its pixel column. */
    void moveToX(units::Fixed x) {
        x_ = x;
        pixelX_ = x.pixels();
    }
    /** To y, and its pixel row. */
    void moveToY(units::Fixed y) {
        y_ = y;
        pixelY_ = y.pixels();
    }

    /** A pixel up and into the air with `walkerSpeed` (sideways 32 times, up 16 times as much); the pixel row stays. */
    void throwUp(units::Fixed walkerSpeed) {
        y_ -= units::Fixed::fromPixels(1);
        vx_ = units::Speed::fromRaw(walkerSpeed.raw() * 32);
        vy_ = units::Speed::fromRaw(walkerSpeed.raw() * 16);
    }

private:
    units::Fixed x_, y_;
    int pixelX_ = 0, pixelY_ = 0;
    units::Speed vx_, vy_;
};

}  // namespace ugh::world::copter
