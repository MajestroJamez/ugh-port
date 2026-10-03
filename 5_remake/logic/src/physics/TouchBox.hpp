// Whether a copter touches a sprite.
#pragma once

#include "data/kinds/Box.hpp"
#include "units/Fixed.hpp"
#include "world/Copter.hpp"
#include "world/Copters.hpp"

namespace ugh::physics {

/**
 * The touch box of a sprite (a passenger, an enemy, a bonus item): a copter touches it when the copter's body
 * overlaps it. The original compares only the copter's top left corner, so the box is grown by the copter's body.
 */
class TouchBox {
public:
    /** The box of a sprite at x, y (its top left corner) with the anchor and the half size of `box`. */
    TouchBox(const data::kinds::Box& box, units::Fixed x, units::Fixed y);
    /** The area the copter's top left corner touches when it is between these corners (a blower's zone). */
    static TouchBox between(units::Fixed left, units::Fixed right, units::Fixed top, units::Fixed bottom);

    bool touches(const world::Copter& copter) const;

    /** The first copter that touches the box; nullptr if none. */
    world::Copter* firstCopterIn(world::Copters& copters) const;

private:
    TouchBox(units::Fixed left, units::Fixed right, units::Fixed top, units::Fixed bottom)
        : left_(left), right_(right), top_(top), bottom_(bottom) {}

    // the corners the copter's top left corner can be between
    units::Fixed left_, right_, top_, bottom_;
};

}  // namespace ugh::physics
