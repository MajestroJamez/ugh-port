// The energy of the copters.
#pragma once

#include "units/Int16.hpp"

namespace ugh::world {

/**
 * The energy the copters share: flying costs it, a bonus item brings it back. Nothing happens when it runs out. The
 * quirk of the original: it is a 16-bit counter - below zero it goes on down and wraps from -32768 to 32767, and a
 * refill compares it unsigned, so a refill while the energy is below zero fills it up.
 */
class Energy {
public:
    static constexpr int FULL = 23099;

    Energy() = default;
    /** At `value` (only the test pilot sets the energy so). */
    explicit Energy(int value) : value_(value) {}

    void fill() { value_ = FULL; }
    void spend(int amount) { value_ = units::Int16(value_ - amount).value(); }

    /** A bonus item's energy, up to FULL (below zero: FULL, see above). */
    void refill(int amount) {
        int sum = units::Int16(value_ + amount).value();
        value_ = sum < 0 || sum > FULL ? FULL : sum;
    }

    int value() const { return value_; }

private:
    int value_ = 0;
};

}  // namespace ugh::world
