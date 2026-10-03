// A passenger waiting for a copter and calling it.
#pragma once

#include "passengers/route/WaitingSpot.hpp"
#include "units/Countdown.hpp"

namespace ugh::passengers::route {

/** A passenger waiting on its pad (or in the water) for a copter: where it waits, and how long it calls or waves. */
class PassengerCall {
public:
    /** It starts to wait: on its way to its spot on the pad. */
    void startWaiting() { spot_ = WaitingSpot::Starting; }
    /** It walks towards its spot. */
    void walkToSpot() { spot_ = WaitingSpot::Walking; }
    /** It stands at its spot; true when it just got there. */
    bool reachSpot() {
        bool arrived = spot_ != WaitingSpot::Reached;
        spot_ = WaitingSpot::Reached;
        return arrived;
    }
    WaitingSpot spot() const { return spot_; }

    /** It calls a copter (or waves at it) for `frames`. */
    void start(int frames) { time_.start(frames); }
    /** One frame of calling; true when the time is up. */
    bool over() { return time_.tick(); }
    int time() const { return time_.remaining(); }

private:
    WaitingSpot spot_ = WaitingSpot::Starting;
    units::Countdown time_;
};

}  // namespace ugh::passengers::route
