// The edges of the play area.
#pragma once

#include "data/levels/ScreenSize.hpp"
#include "units/Fixed.hpp"

namespace ugh::world::scenery {

/** The edges of the play area: a falling or flying thing past them is gone (its top left corner). */
class Screen {
public:
    static constexpr units::Fixed LEFT = units::Fixed::fromPixels(-16);
    static constexpr units::Fixed RIGHT = units::Fixed::fromPixels(data::levels::ScreenSize::WIDTH);
    static constexpr units::Fixed BOTTOM = units::Fixed::fromPixels(data::levels::ScreenSize::HEIGHT);
    /** The middle column. */
    static constexpr units::Fixed MIDDLE = units::Fixed::fromPixels(data::levels::ScreenSize::WIDTH / 2);
    /** The flyer is wider: it is gone only 32 px past the left edge. */
    static constexpr units::Fixed FLYER_LEFT = units::Fixed::fromPixels(-32);

    /** x is past the left or the right edge. */
    static bool pastSide(units::Fixed x, units::Fixed left = LEFT) { return x <= left || x >= RIGHT; }
};

}  // namespace ugh::world::scenery
