// The passengers a level still has to deliver.
#pragma once

namespace ugh::world {

/** How many passengers of the level are still to be delivered; the level is done with the last one. */
class Delivery {
public:
    void start(int passengers) {
        left_ = passengers;
        done_ = false;
    }
    /** A passenger finished its route; true when it was the last. */
    bool finishOne() {
        if (left_ == 0) return false;
        if (--left_ != 0) return false;
        done_ = true;
        return true;
    }
    int left() const { return left_; }
    bool done() const { return done_; }

private:
    int left_ = 0;
    bool done_ = false;
};

}  // namespace ugh::world
