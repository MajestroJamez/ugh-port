// Frames left until something happens.
#pragma once

#include "units/Int16.hpp"

namespace ugh::units {

/**
 * Frames left until something happens: each tick counts one down, and the tick that reaches zero is the one that
 * acts. A countdown started at n acts on its n-th tick; one started at 0 runs through all 65536 values first.
 */
class Countdown {
public:
    constexpr Countdown() = default;

    constexpr void start(Int16 frames) { remaining_ = frames; }

    /** One frame: true when the countdown just reached zero. */
    constexpr bool tick() {
        remaining_ -= 1;
        return remaining_ == Int16(0);
    }

    constexpr Int16 remaining() const { return remaining_; }

private:
    Int16 remaining_;
};

}  // namespace ugh::units
