// Whether a copter touches a sprite.
#pragma once

#include "core/Fixed.hpp"
#include "data/Box.hpp"
#include "model/Copter.hpp"
#include "model/Level.hpp"

namespace ugh::physics {

/**
 * 113b:2276, 22f1, 2207 - the touch box of a sprite (a passenger, an enemy, a bonus item): a copter touches it when
 * the copter's body overlaps it. The original compares only the copter's top left corner, so the box is grown by
 * the size of the copter's body.
 */
class TouchBox {
public:
    /** The box of a sprite at x, y (its top left corner), with the anchor and the half size of `box`. */
    TouchBox(const data::Box& box, core::Fixed x, core::Fixed y);

    bool touches(const model::Copter& copter) const;

    /** The first copter of the level touching the box; Level::NONE when there is none. */
    int firstCopterIn(const model::Level& level) const;

private:
    // the corners the copter's top left corner can be between
    core::Fixed left_, right_, top_, bottom_;
};

}  // namespace ugh::physics
