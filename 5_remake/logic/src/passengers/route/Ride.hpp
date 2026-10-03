// A passenger riding in a copter.
#pragma once

#include <optional>

namespace ugh::passengers::route {

/** A passenger riding in a copter: the copter that carries it and how long a delivery still counts as quick. */
class Ride {
public:
    /** It gets into the copter of `player`; a delivery within `quickFrames` drops a bonus item. */
    void start(int player, int quickFrames) {
        carrier_ = player;
        quickDeliveryTime_ = quickFrames;
    }
    /** The copter that carries it. */
    std::optional<int> carrier() const { return carrier_; }

    /** One frame of the ride. */
    void tick() {
        if (quickDeliveryTime_ > 0) quickDeliveryTime_ -= 1;
    }
    bool quick() const { return quickDeliveryTime_ != 0; }
    int quickDeliveryTime() const { return quickDeliveryTime_; }

private:
    std::optional<int> carrier_;
    int quickDeliveryTime_ = 0;
};

}  // namespace ugh::passengers::route
