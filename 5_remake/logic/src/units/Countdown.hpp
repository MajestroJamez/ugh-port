// Frames left until something happens.
#pragma once

namespace ugh::units {

/**
 * Frames left until something happens: each tick counts one down, and the tick that reaches zero is the one that
 * acts. A countdown started at n acts on its n-th tick. (Started at 0, the original's 16-bit countdown would act after
 * 65536 frames - a quarter of an hour; the game never starts one at 0.)
 */
class Countdown {
public:
    constexpr Countdown() = default;

    constexpr void start(int frames) { remaining_ = frames; }

    /** One frame: true when the countdown just reached zero. */
    constexpr bool tick() {
        remaining_ -= 1;
        return remaining_ == 0;
    }

    constexpr int remaining() const { return remaining_; }

private:
    int remaining_ = 0;
};

}  // namespace ugh::units
