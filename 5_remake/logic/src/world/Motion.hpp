// Where a copter is and how fast it moves.
#pragma once

#include "units/Fixed.hpp"
#include "units/Speed.hpp"

namespace ugh::world {

/**
 * Where a copter is and how fast it moves. The pixel position is what the collision probe sees: it follows x and y,
 * except that a copter thrown by a walker keeps its pixel row for a frame (`thrown`).
 */
class Motion {
public:
    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    int pixelX() const { return pixelX_; }
    int pixelY() const { return pixelY_; }
    units::Speed speedX() const { return vx_; }
    units::Speed speedY() const { return vy_; }

    void setSpeed(units::Speed vx, units::Speed vy) {
        vx_ = vx;
        vy_ = vy;
    }
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

protected:
    /** A pixel up and into the air with `walkerSpeed` (sideways 32 times, up 16 times as much); the pixel row stays. */
    void thrown(int walkerSpeed) {
        y_ -= units::Fixed::fromPixels(1);
        vx_ = units::Speed::fromRaw(walkerSpeed << 5);
        vy_ = units::Speed::fromRaw(walkerSpeed << 4);
    }
    /** Anywhere, the pixel position too (the test pilot). */
    void place(units::Fixed x, units::Fixed y, int pixelX, int pixelY, units::Speed vx, units::Speed vy) {
        x_ = x;
        y_ = y;
        pixelX_ = pixelX;
        pixelY_ = pixelY;
        setSpeed(vx, vy);
    }

private:
    units::Fixed x_, y_;
    int pixelX_ = 0, pixelY_ = 0;
    units::Speed vx_, vy_;
};

}  // namespace ugh::world
