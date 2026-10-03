// A walker charging at a copter.
#pragma once

#include "units/Fixed.hpp"
#include "world/Facing.hpp"

namespace ugh::enemies::walker {

/** How much faster than its walk a walker charges (Fixed per frame): 1/32 px more every frame, the way it faces. */
class Charge {
public:
    void start() { speed_ = units::Fixed(); }
    /** One frame of the charge: faster towards `facing`. */
    void faster(world::Facing facing) { speed_ += units::Fixed::fromRaw(facing == world::Facing::Right ? 1 : -1); }
    units::Fixed speed() const { return speed_; }

private:
    units::Fixed speed_;
};

}  // namespace ugh::enemies::walker
