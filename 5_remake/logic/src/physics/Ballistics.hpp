// How a thrown thing falls.
#pragma once

#include <optional>

#include "units/Fixed.hpp"
#include "units/Int16.hpp"
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
        units::Fixed x, y;             // top left corner
        units::Int16 anchorX, anchorY; // from the top left corner to its middle and its bottom, in pixels
        units::Fixed speedX;           // per frame
        units::Int16 fallSpeed;        // 1/32 px per frame; negative: up
    };

    /**
     * How it lands on a pad. A passenger lands when its feet were on or above the surface; a bonus item only when it
     * was above it, and its middle may be a pixel past the right end of the pad.
     */
    enum class Landing { Passenger, BonusItem };

    /** What came of the frame. */
    enum class Result { Flying, Landed, Gone };

    Ballistics(units::Int16 gravity, Landing landing) : gravity_(gravity), landing_(landing) {}

    /** One frame of the fall; on landing the body stands on the pad's surface. */
    Result fall(Body& body, const world::Level& level) const;

private:
    units::Int16 gravity_;
    Landing landing_;

    bool landsOn(const data::PadDefinition& pad, units::Int16 bottomBefore, units::Int16 bottom, units::Int16 middle) const;
};

}  // namespace ugh::physics
