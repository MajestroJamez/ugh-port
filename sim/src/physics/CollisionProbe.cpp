#include "physics/CollisionProbe.hpp"

#include <array>

#include "data/CollisionMask.hpp"

namespace ugh::physics {

namespace {

using core::Fixed;

constexpr int ROW = data::CollisionMask::WIDTH;   // pixels per row of the collision mask

struct Point {
    int x, y;
};

/** The points of the copter's outline, in pixels from the probe origin: the corners and the sides. */
constexpr std::array<Point, 10> OUTLINE = {
    {{0, 0}, {12, 0}, {20, 0}, {0, 19}, {12, 19}, {20, 19}, {0, 6}, {20, 6}, {0, 12}, {20, 12}}};
constexpr int ORIGIN_OFFSET_X = 5;

constexpr Fixed ONE_PIXEL = Fixed::fromPixels(1);

}  // namespace

/** Game.kt moveHorizontally / moveVertically: the loop over the pixels of the move. */
std::optional<Fixed> CollisionProbe::stopOnTheWay(const model::Copter& copter, Axis axis, Fixed from, Fixed to) const {
    int step = axis == Axis::Horizontal ? 1 : ROW;
    int origin = originOf(copter);
    if (to <= from) {
        // left or up: only the pixel next to the copter (the quirk above)
        if (hits(origin - step)) return from.wholePixel() + ONE_PIXEL;
        return std::nullopt;
    }
    for (Fixed position = from;; position += ONE_PIXEL) {
        origin += step;
        if (hits(origin)) return position.wholePixel();
        if (position + ONE_PIXEL >= to) return std::nullopt;
    }
}

bool CollisionProbe::hits(int origin) const {
    for (Point point : OUTLINE)
        if (level_.solid(origin + point.y * ROW + point.x)) return true;
    return false;
}

int CollisionProbe::originOf(const model::Copter& copter) {
    return copter.pixelY().value() * ROW + copter.pixelX().value() + ORIGIN_OFFSET_X;
}

}  // namespace ugh::physics
