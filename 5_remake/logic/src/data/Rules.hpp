// The rules that depend on the difficulty.
#pragma once

#include <array>

#include "data/BonusKind.hpp"
#include "data/Difficulty.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** The rules of the game that the data sets. */
struct Rules {
    std::array<units::Int16, 3> crashLimits;        // by difficulty: a bounce this hard crashes the copter
    std::array<units::Int16, 3> multiplierLimits;   // by difficulty: the score multiplier goes no higher
    const BonusKind* quickDeliveryBonus = nullptr;   // the bonus item a quick delivery drops

    units::Int16 crashLimit(Difficulty d) const { return crashLimits[static_cast<int>(d)]; }
    units::Int16 multiplierLimit(Difficulty d) const { return multiplierLimits[static_cast<int>(d)]; }
};

}  // namespace ugh::data
