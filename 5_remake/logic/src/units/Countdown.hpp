// Frames left until something happens.
#pragma once

namespace ugh::units {

/**
 * Frames left until something happens: each tick counts one down, and the tick that reaches zero is the one that
 * acts. A countdown started at n acts on its n-th tick. One started at 0, or never started, does not act (the
 * original's 16-bit countdown would, after 65536 frames - a quarter of an hour): the logic starts every countdown above
 * 0 before it ticks it (the states in their entry actions, `world::Animator::restart`).
 *
 * `tickToZero` is the other kind of countdown of the original: it stops at zero and stays there, and zero means
 * "due" (a delay of 0 is none).
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

    /** One frame down to zero, where it stays: true at zero (also when it was there already). */
    constexpr bool tickToZero() {
        if (remaining_ > 0) remaining_ -= 1;
        return remaining_ == 0;
    }

    constexpr int remaining() const { return remaining_; }

private:
    int remaining_ = 0;
};

}  // namespace ugh::units
