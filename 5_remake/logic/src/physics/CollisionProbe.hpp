// Where a moving copter hits the background.
#pragma once

#include <optional>

#include "units/Fixed.hpp"
#include "world/Level.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::physics {

/**
 * Ten points of the copter's outline (`world::copter::CopterShape::OUTLINE`) tested against the collision mask of the
 * level. The physics moves a copter pixel by pixel along one axis and stops it at the first pixel where a point of its
 * outline would be in something solid.
 *
 * Quirk of the original: moving left or up, only the pixel next to the copter is probed, however far the copter
 * moves in the frame. A fast copter can fly through a thin wall to the left or upwards, never to the right or down.
 */
class CollisionProbe {
public:
    enum class Axis { Horizontal, Vertical };

    explicit CollisionProbe(const world::Level& level) : level_(level) {}

    /**
     * The copter moves along `axis` from `from` (its position on that axis now) to `to`: where it stops when it hits
     * something on the way; nothing when it gets there.
     */
    std::optional<units::Fixed> stopOnTheWay(const world::copter::Copter& copter, Axis axis, units::Fixed from,
                                             units::Fixed to) const;

    /**
     * How many whole pixels the copter can move along `axis` from where it is (`direction` 1: right or down, -1: left
     * or up) before a point of its outline would be in something solid, looking at most `most` pixels far; nothing when
     * the way is clear so far. Every pixel on the way is probed, either way (only a look ahead, not the move).
     */
    std::optional<int> clearance(const world::copter::Copter& copter, Axis axis, int direction, int most) const;

private:
    const world::Level& level_;

    /** A point of the outline is solid with the outline starting at x, y (pixels). */
    bool hits(int x, int y) const;
};

}  // namespace ugh::physics
