// An enemy stunned by a passenger.
#pragma once

#include "units/Countdown.hpp"

namespace ugh::enemies {

/** How long an enemy (a walker, a blower) stays stunned after a standing passenger fell onto it. */
class Stun {
public:
    /** Frames it stays stunned. */
    static constexpr int TIME = 350;

    void start() { time_.start(TIME); }
    /** One frame stunned; true when it is over. */
    bool over() { return time_.tick(); }
    /** Frames left. */
    int time() const { return time_.remaining(); }

private:
    units::Countdown time_;
};

}  // namespace ugh::enemies
