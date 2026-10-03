// Whether a copter touches a sprite.
#pragma once

#include <optional>

#include "data/Box.hpp"
#include "units/Fixed.hpp"
#include "world/Copter.hpp"
#include "world/Level.hpp"

namespace ugh::physics {

/**
 * The touch box of a sprite (a passenger, an enemy, a bonus item): a copter touches it when the copter's body
 * overlaps it. The original compares only the copter's top left corner, so the box is grown by the copter's body.
 */
class TouchBox {
public:
    /** The box of a sprite at x, y (its top left corner) with the anchor and the half size of `box`. */
    TouchBox(const data::Box& box, units::Fixed x, units::Fixed y);

    bool touches(const world::Copter& copter) const;

    /** The first copter of the level that touches the box. */
    std::optional<int> firstCopterIn(const world::Level& level) const;

private:
    // the corners the copter's top left corner can be between
    units::Fixed left_, right_, top_, bottom_;
};

}  // namespace ugh::physics
