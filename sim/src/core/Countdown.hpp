// The countdown of the original: DEC, and done at zero.
#pragma once

#include "core/Word.hpp"

namespace ugh::core {

/**
 * Frames left until something happens. The original counts down with DEC and acts when the word reaches zero, so a
 * countdown started at n is done on the n-th tick, and one started at 0 runs through the whole word (65536 ticks).
 */
class Countdown {
public:
    constexpr Countdown() = default;
    constexpr explicit Countdown(Word remaining) : remaining_(remaining) {}

    constexpr void start(Word frames) { remaining_ = frames; }

    /** One frame: true when the countdown just reached zero. */
    constexpr bool tick() { return --remaining_ == 0; }

    constexpr Word remaining() const { return remaining_; }

private:
    Word remaining_;
};

}  // namespace ugh::core
