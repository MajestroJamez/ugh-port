#include "physics/CollisionProbe.hpp"

namespace ugh::physics {

namespace {

using units::Fixed;
using world::copter::CopterShape;

constexpr Fixed ONE_PIXEL = Fixed::fromPixels(1);

}  // namespace

std::optional<Fixed> CollisionProbe::stopOnTheWay(const world::copter::Copter& copter, Axis axis, Fixed from,
                                                 Fixed to) const {
    int x = copter.motion().pixelX() + CopterShape::OUTLINE_LEFT, y = copter.motion().pixelY();
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

std::optional<int> CollisionProbe::clearance(const world::copter::Copter& copter, Axis axis, int direction,
                                            int most) const {
    const int x = copter.motion().pixelX() + CopterShape::OUTLINE_LEFT, y = copter.motion().pixelY();
    const int dx = axis == Axis::Horizontal ? direction : 0, dy = axis == Axis::Vertical ? direction : 0;
    for (int moved = 1; moved <= most; moved++)
        if (hits(x + moved * dx, y + moved * dy)) return moved - 1;
    return std::nullopt;
}

bool CollisionProbe::hits(int x, int y) const {
    for (CopterShape::Point point : CopterShape::OUTLINE)
        if (level_.solid(x + point.x, y + point.y)) return true;
    return false;
}

}  // namespace ugh::physics
