// The energy of the copters.
#pragma once

#include "units/Int16.hpp"

namespace ugh::world {

/** The energy the copters share: flying costs it, a bonus item brings it back. Nothing happens when it runs out. */
class Energy {
public:
    static constexpr units::Int16 FULL = 23099;

    void fill() { value_ = FULL; }
    void spend(units::Int16 amount) { value_ -= amount; }

    /** A bonus item's energy, up to FULL (compared unsigned). */
    void refill(units::Int16 amount) {
        units::Int16 sum = amount + value_;
        value_ = units::Int16::unsignedLess(FULL, sum) ? FULL : sum;
    }

    units::Int16 value() const { return value_; }
    /** The test pilot of the replays sets it (Cheats only). */
    void setByTestPilot(units::Int16 value) { value_ = value; }

private:
    units::Int16 value_;
};

}  // namespace ugh::world
