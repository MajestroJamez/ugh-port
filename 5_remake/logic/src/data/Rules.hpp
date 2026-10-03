// The rules that depend on the difficulty.
#pragma once

#include <array>

#include "data/Difficulty.hpp"
#include "data/kinds/BonusKind.hpp"

namespace ugh::data {

/** The rules of the game that the data sets. */
struct Rules {
    std::array<int, DIFFICULTY_COUNT> crashLimits;       // by difficulty: a bounce this hard crashes the copter
    std::array<int, DIFFICULTY_COUNT> multiplierLimits;  // by difficulty: the score multiplier goes no higher
    const kinds::BonusKind* quickDeliveryBonus = nullptr;   // the bonus item a quick delivery drops

    int crashLimit(Difficulty d) const { return crashLimits[static_cast<int>(d)]; }
    int multiplierLimit(Difficulty d) const { return multiplierLimits[static_cast<int>(d)]; }
};

}  // namespace ugh::data
