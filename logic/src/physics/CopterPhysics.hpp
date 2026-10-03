// How a copter flies.
#pragma once

#include "units/Fixed.hpp"
#include "units/Int16.hpp"
#include "units/Speed.hpp"
#include "world/Copter.hpp"
#include "world/PlayContext.hpp"

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

    void fly(int player);

private:
    /** Where the copter is against the water surface. */
    enum class Depth { Above, Surface, Below };

    const world::PlayContext& context_;
    units::Int16 impact_;   // the hardest bounce of this frame

    Depth depthOf(const world::Copter& copter) const;
    void blowWithWind(world::Copter& copter, Depth depth);
    void steer(world::Copter& copter);
    void moveHorizontally(world::Copter& copter);
    void liftAndFall(world::Copter& copter, Depth depth);
    void moveVertically(world::Copter& copter, Depth depth);
    void bounceVertically(world::Copter& copter, units::Fixed y);
    void touchDownOnPad(world::Copter& copter, units::Fixed y);
    void checkCrash(int player);

    /** A bounce: half the speed back; the impact is the speed it had (twice the half, as the original computes). */
    static units::Int16 bounce(units::Speed& speed);
};

}  // namespace ugh::physics
