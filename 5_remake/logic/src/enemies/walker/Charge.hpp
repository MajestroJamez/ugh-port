// A walker charging at a copter.
#pragma once

#include "world/Facing.hpp"

namespace ugh::enemies::walker {

/** How much faster than its walk a walker charges (Fixed per frame): one more every frame, the way it faces. */
class Charge {
public:
    void start() { speed_ = 0; }
    /** One frame of the charge: faster towards `facing`. */
    void faster(world::Facing facing) { speed_ += facing == world::Facing::Right ? 1 : -1; }
    int speed() const { return speed_; }

private:
    int speed_ = 0;
};

}  // namespace ugh::enemies::walker
