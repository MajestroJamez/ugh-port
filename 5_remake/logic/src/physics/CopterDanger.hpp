// How dangerous a copter's speed is.
#pragma once

#include "units/Speed.hpp"
#include "world/Level.hpp"
#include "world/copter/Copter.hpp"

namespace ugh::physics {

/**
 * What a copter's speed would cost if it hit something now, along each axis (only a look: nothing changes). A bounce
 * off the collision mask at a speed whose impact (`CopterPhysics::impactOf`) reaches the session's crash limit crashes
 * the copter - a wall across, a floor (a pad too) or a ceiling of rock up and down. The edges of the screen stop a
 * copter without a bounce (no crash), the water's surface brakes it (no bounce): so along an axis the speed is a danger
 * only with rock ahead - before the edge of the screen across or up, before the water's surface down, under the surface
 * within the way the water brakes it from a crash (under water up only to the surface, where it stops).
 *
 * Quirk of the original (CollisionProbe): a copter going left or up fast may fly through a wall of one pixel without
 * touching it; the look ahead counts that wall as rock all the same.
 */
struct CopterDanger {
    /** Along one axis. */
    struct Axis {
        units::Speed speed;   // right or down positive
        int impact = 0;       // of a bounce at that speed
        bool rock = false;    // rock ahead, that a bounce at that speed would come from
    };

    int crashLimit = 0;   // the session's: a bounce this hard or harder crashes the copter
    Axis across;
    Axis upDown;

    /** Along `axis` the copter heads for rock fast enough to crash into it. */
    bool crashes(const Axis& axis) const { return axis.rock && axis.impact >= crashLimit; }

    /** The danger of `copter` in `level` at the crash limit `crashLimit`. */
    static CopterDanger of(const world::Level& level, int crashLimit, const world::copter::Copter& copter);
};

}  // namespace ugh::physics
