// How a thrown thing falls.
#pragma once

#include <optional>

#include "units/Fixed.hpp"
#include "world/Level.hpp"

namespace ugh::physics {

/**
 * The fall of a thing thrown into the air - the standing passenger let go by a copter, a bonus item: one frame moves
 * it sideways, pulls it down by `gravity`, and it lands on a pad it comes down onto, or is gone off the screen.
 */
class Ballistics {
public:
    /** Where the thing is, in which point it lands, and how fast it goes. */
    struct Body {
        units::Fixed x, y;              // top left corner
        int anchorX = 0, anchorY = 0;   // from the top left corner to its middle and its bottom, in pixels
        units::Fixed speedX;            // per frame
        int fallSpeed = 0;              // 1/32 px per frame; negative: up
    };

    /**
     * How it lands on a pad. A passenger lands when its feet were on or above the surface; a bonus item only when it
     * was above it, and its middle may be a pixel past the right end of the pad.
     */
    enum class Landing { Passenger, BonusItem };

    /** What came of the frame. */
    enum class Result { Flying, Landed, Gone };

    Ballistics(int gravity, Landing landing) : gravity_(gravity), landing_(landing) {}

    /** One frame of the fall; on landing the body stands on the pad's surface. */
    Result fall(Body& body, const world::Level& level) const;

private:
    int gravity_ = 0;
    Landing landing_;

    bool landsOn(const data::PadDefinition& pad, int bottomBefore, int bottom, int middle) const;
};

}  // namespace ugh::physics
