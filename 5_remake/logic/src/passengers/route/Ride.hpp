// A passenger riding in a copter.
#pragma once

#include "units/Countdown.hpp"
#include "world/Copter.hpp"

namespace ugh::passengers::route {

/** A passenger riding in a copter: the copter that carries it and how long a delivery still counts as quick. */
class Ride {
public:
    /** It gets into `copter`; a delivery within `quickFrames` drops a bonus item. */
    void start(world::Copter& copter, int quickFrames) {
        carrier_ = &copter;
        quickDelivery_.start(quickFrames);
    }
    /** The copter that carries it; nullptr before its first ride. */
    world::Copter* carrier() const { return carrier_; }

    /** One frame of the ride. */
    void tick() { quickDelivery_.tickToZero(); }
    bool quick() const { return quickDelivery_.remaining() != 0; }
    int quickDeliveryTime() const { return quickDelivery_.remaining(); }

private:
    world::Copter* carrier_ = nullptr;
    units::Countdown quickDelivery_;
};

}  // namespace ugh::passengers::route
