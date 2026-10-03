#include "physics/CollisionProbe.hpp"

#include <array>
#include <string>

#include "core/Audit.hpp"

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
    auditX_ = copter.pixelX().value() + ORIGIN_OFFSET_X;
    if (to <= from) {
        // left or up: only the pixel next to the copter (the quirk above)
        if (axis == Axis::Horizontal) auditX_ -= 1;
        bool hit = hits(origin - step);
        // what a full sweep (like right / down) would give
        int pixels = from.pixels().value() - to.pixels().value();
        if (pixels > 1) {
            core::audit::count("Q5 probe left/up more than 1 px");
            bool sweep = false;
            for (int k = 1; k <= pixels && !sweep; k++) {
                auditX_ = copter.pixelX().value() + ORIGIN_OFFSET_X - (axis == Axis::Horizontal ? k : 0);
                sweep = hits(origin - k * step);
            }
            if (sweep != hit)
                core::audit::count(std::string("Q5 probe quirk changes outcome ") +
                                   (axis == Axis::Horizontal ? "left" : "up"));
        }
        if (hit) return from.wholePixel() + ONE_PIXEL;
        return std::nullopt;
    }
    for (Fixed position = from;; position += ONE_PIXEL) {
        origin += step;
        if (axis == Axis::Horizontal) auditX_++;
        if (hits(origin)) return position.wholePixel();
        if (position + ONE_PIXEL >= to) return std::nullopt;
    }
}

bool CollisionProbe::hits(int origin) const {
    bool any = false;
    for (Point point : OUTLINE) {
        int x = auditX_ + point.x;
        int index = origin + point.y * ROW + point.x;
        bool solid = level_.solid(index);
        if (x < 0 || x >= ROW) core::audit::count(std::string("Q5 probe point outside row ") + (solid ? "solid" : "empty"));
        if (index < 0 || index >= ROW * data::CollisionMask::HEIGHT) core::audit::count("Q5 probe point outside mask");
        if (solid) any = true;
    }
    return any;
}

int CollisionProbe::originOf(const model::Copter& copter) {
    return copter.pixelY().value() * ROW + copter.pixelX().value() + ORIGIN_OFFSET_X;
}

}  // namespace ugh::physics
