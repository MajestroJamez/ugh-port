#include "physics/CollisionProbe.hpp"

#include <array>

namespace ugh::physics {

namespace {

using units::Fixed;

struct Point {
    int x, y;
};

/** The points of the copter's outline, in pixels from the probe origin: the corners and the sides. */
constexpr std::array<Point, 10> OUTLINE = {
    {{0, 0}, {12, 0}, {20, 0}, {0, 19}, {12, 19}, {20, 19}, {0, 6}, {20, 6}, {0, 12}, {20, 12}}};

/** The probe origin is 5 px right of the copter's left edge. */
constexpr int ORIGIN_OFFSET_X = 5;

constexpr Fixed ONE_PIXEL = Fixed::fromPixels(1);

}  // namespace

std::optional<Fixed> CollisionProbe::stopOnTheWay(const world::Copter& copter, Axis axis, Fixed from, Fixed to) const {
    int x = copter.pixelX() + ORIGIN_OFFSET_X, y = copter.pixelY();
    int dx = axis == Axis::Horizontal ? 1 : 0, dy = axis == Axis::Vertical ? 1 : 0;
    if (to <= from) {
        // left or up: only the pixel next to the copter (the quirk above)
        if (hits(x - dx, y - dy)) return from.wholePixel() + ONE_PIXEL;
        return std::nullopt;
    }
    for (Fixed position = from;; position += ONE_PIXEL) {
        x += dx;
        y += dy;
        if (hits(x, y)) return position.wholePixel();
        if (position + ONE_PIXEL >= to) return std::nullopt;
    }
}

bool CollisionProbe::hits(int x, int y) const {
    for (Point point : OUTLINE)
        if (level_.solid(x + point.x, y + point.y)) return true;
    return false;
}

}  // namespace ugh::physics
