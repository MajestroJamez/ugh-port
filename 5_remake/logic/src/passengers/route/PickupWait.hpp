// A passenger waiting to be picked up.
#pragma once

#include "passengers/route/WaitingSpot.hpp"
#include "units/Countdown.hpp"

namespace ugh::passengers::route {

/**
 * A passenger waiting on its pad (or in the water) to be picked up by a copter: where it waits, and how long it calls
 * the copter or waves at it.
 */
class PickupWait {
public:
    /** How long a passenger calls before it walks (or swims) to the copter. */
    static constexpr int CALL_TIME = 140;
    /** How long it waves impatiently (no copter with room) before it waits again (or sinks). */
    static constexpr int WAVE_TIME = 140;

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

    /** It calls a copter for CALL_TIME. */
    void startCalling() { time_.start(CALL_TIME); }
    /** It waves impatiently for WAVE_TIME. */
    void startWaving() { time_.start(WAVE_TIME); }
    /** One frame of calling or waving; true when the time is up. */
    bool tick() { return time_.tick(); }
    int time() const { return time_.remaining(); }

private:
    WaitingSpot spot_ = WaitingSpot::Starting;
    units::Countdown time_;
};

}  // namespace ugh::passengers::route
