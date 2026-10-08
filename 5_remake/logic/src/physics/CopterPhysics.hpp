// How a copter flies.
#pragma once

#include "units/Fixed.hpp"
#include "units/Speed.hpp"
#include "world/PlayContext.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::physics {

/**
 * One frame of a copter's flight: the wind and the arrow keys push it sideways, gravity pulls it down and the water
 * lifts it, the up key lifts it and the down key dives; it moves pixel by pixel against the collision mask, bounces
 * off what it hits, lands on a pad it comes down on softly, and crashes when it hits anything too hard. Flying costs
 * energy.
 */
class CopterPhysics {
public:
    explicit CopterPhysics(const world::PlayContext& context) : context_(context) {}

    /** No copter flies faster, across or up and down (1/64 Fixed per frame). */
    static constexpr units::Speed TOP_SPEED = units::Speed::fromRaw(6144);
    /** How far a copter can go (its top left corner): it stops there, no bounce. */
    static constexpr units::Fixed LEFT_EDGE = units::Fixed::fromPixels(-16), RIGHT_EDGE = units::Fixed::fromPixels(304);
    static constexpr units::Fixed TOP_EDGE = units::Fixed::fromRaw(-608), BOTTOM_EDGE = units::Fixed::fromRaw(6112);
    /** Under water a copter sinking slows down by this much a frame. */
    static constexpr units::Speed WATER_BRAKE = units::Speed::fromRaw(193);

    /** One frame of the copter's flight. */
    void fly(world::copter::Copter& copter);

    /**
     * How hard a bounce at `speed` is: the speed it had, as the original computes it from the half it bounces back with
     * (an odd speed down or to the right one more, up or to the left one less). A bounce this hard as the session's
     * crash limit or harder crashes the copter.
     */
    static constexpr int impactOf(units::Speed speed) {
        const int impact = ((-speed) >> 1).raw() * 2;
        return impact < 0 ? -impact : impact;
    }

private:
    /** Where the copter is against the water surface. */
    enum class Depth { Above, Surface, Below };

    const world::PlayContext& context_;
    int impact_ = 0;   // the hardest bounce of this frame

    Depth depthOf(const world::copter::Copter& copter) const {
        int depth = copter.depthIn(context_.level.water().row());
        return depth < 0 ? Depth::Above : depth == 0 ? Depth::Surface : Depth::Below;
    }
    void blowWithWind(world::copter::Copter& copter, Depth depth);
    void steer(world::copter::Copter& copter);
    void moveHorizontally(world::copter::Copter& copter);
    void liftAndFall(world::copter::Copter& copter, Depth depth);
    void moveVertically(world::copter::Copter& copter, Depth depth);
    void bounceVertically(world::copter::Copter& copter, units::Fixed y);
    void touchDownOnPad(world::copter::Copter& copter, units::Fixed y);
    void checkCrash(const world::copter::Copter& copter);

    /** A bounce: half the speed back; the impact is the speed it had (twice the half, as the original computes). */
    static int bounce(units::Speed& speed);
};

}  // namespace ugh::physics
