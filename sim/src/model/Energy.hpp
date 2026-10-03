// The energy the copters share.
#pragma once

#include "core/Word.hpp"

namespace ugh::model {

/**
 * The energy of the copters (shared in the team mode): flying costs it, an energy bonus item gives it back. Nothing
 * happens when it runs out; the status line shows it.
 */
class Energy {
public:
    static constexpr core::Word FULL = 0x5a3b;

    Energy() = default;
    explicit Energy(core::Word value) : value_(value) {}

    void fill() { value_ = FULL; }
    void spend(core::Word amount) { value_ -= amount; }

    /** 113b:2ca9 - adds a bonus item's energy, up to FULL (compared unsigned, as the original does). */
    void refill(core::Word amount) {
        core::Word sum = amount + value_;
        value_ = core::Word::unsignedLess(FULL, sum) ? FULL : sum;
    }

    core::Word value() const { return value_; }

private:
    core::Word value_;
};

}  // namespace ugh::model
