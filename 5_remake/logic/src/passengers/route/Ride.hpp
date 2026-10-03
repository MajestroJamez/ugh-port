// A passenger riding in a copter.
#pragma once

#include "world/Copter.hpp"

namespace ugh::passengers::route {

/** A passenger riding in a copter: the copter that carries it and how long a delivery still counts as quick. */
class Ride {
public:
    /** It gets into `copter`; a delivery within `quickFrames` drops a bonus item. */
    void start(world::Copter& copter, int quickFrames) {
        carrier_ = &copter;
        quickDeliveryTime_ = quickFrames;
    }
    /** The copter that carries it; nullptr before its first ride. */
    world::Copter* carrier() const { return carrier_; }

    /** One frame of the ride. */
    void tick() {
        if (quickDeliveryTime_ > 0) quickDeliveryTime_ -= 1;
    }
    bool quick() const { return quickDeliveryTime_ != 0; }
    int quickDeliveryTime() const { return quickDeliveryTime_; }

private:
    world::Copter* carrier_ = nullptr;
    int quickDeliveryTime_ = 0;
};

}  // namespace ugh::passengers::route
