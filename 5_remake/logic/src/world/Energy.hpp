// The energy of the copters.
#pragma once

#include <algorithm>

namespace ugh::world {

/** The energy the copters share: flying costs it, a bonus item brings it back. Nothing happens when it runs out. */
class Energy {
public:
    static constexpr int FULL = 23099;

    void fill() { value_ = FULL; }
    void spend(int amount) { value_ -= amount; }

    /** A bonus item's energy, up to FULL. */
    void refill(int amount) { value_ = std::min(value_ + amount, FULL); }

    int value() const { return value_; }
    /** The test pilot of the replays sets it (Cheats only). */
    void setByTestPilot(int value) { value_ = value; }

private:
    int value_ = 0;
};

}  // namespace ugh::world
