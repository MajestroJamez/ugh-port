// The edges of the play area.
#pragma once

#include "units/Fixed.hpp"

namespace ugh::world {

/** The edges of the play area: a falling or flying thing past them is gone (its top left corner). */
class Screen {
public:
    static constexpr units::Fixed LEFT = units::Fixed::fromPixels(-16);
    static constexpr units::Fixed RIGHT = units::Fixed::fromPixels(320);
    static constexpr units::Fixed BOTTOM = units::Fixed::fromPixels(192);
    /** The flyer is wider: it is gone only 32 px past the left edge. */
    static constexpr units::Fixed FLYER_LEFT = units::Fixed::fromPixels(-32);

    /** x is past the left or the right edge. */
    static bool pastSide(units::Fixed x, units::Fixed left = LEFT) { return x <= left || x >= RIGHT; }
};

}  // namespace ugh::world
