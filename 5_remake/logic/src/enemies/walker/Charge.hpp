// A walker charging at a copter.
#pragma once

#include "data/kinds/Facing.hpp"
#include "units/Fixed.hpp"

namespace ugh::enemies::walker {

/** How much faster than its walk a walker charges (Fixed per frame): 1/32 px more every frame, the way it faces. */
class Charge {
public:
    void start() { speed_ = units::Fixed(); }
    /** One frame of the charge: faster towards `facing`. */
    void faster(data::kinds::Facing facing) {
        speed_ += units::Fixed::fromRaw(facing == data::kinds::Facing::Right ? 1 : -1);
    }
    units::Fixed speed() const { return speed_; }

private:
    units::Fixed speed_;
};

}  // namespace ugh::enemies::walker
