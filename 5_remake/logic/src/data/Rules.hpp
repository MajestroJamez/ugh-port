// The rules that depend on the difficulty.
#pragma once

#include <array>

#include "data/BonusKind.hpp"
#include "data/Difficulty.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** The rules of the game that the data sets. */
class Rules {
public:
    Rules(std::array<units::Int16, 3> crashLimits, std::array<units::Int16, 3> multiplierLimits,
          const BonusKind& quickDeliveryBonus)
        : crashLimits_(crashLimits), multiplierLimits_(multiplierLimits), quickDeliveryBonus_(&quickDeliveryBonus) {}

    /** A bounce this hard crashes the copter. */
    units::Int16 crashLimit(Difficulty d) const { return crashLimits_[static_cast<int>(d)]; }
    /** The score multiplier goes no higher. */
    units::Int16 multiplierLimit(Difficulty d) const { return multiplierLimits_[static_cast<int>(d)]; }
    /** The bonus item a quick delivery drops. */
    const BonusKind& quickDeliveryBonus() const { return *quickDeliveryBonus_; }

private:
    std::array<units::Int16, 3> crashLimits_;
    std::array<units::Int16, 3> multiplierLimits_;
    const BonusKind* quickDeliveryBonus_;
};

}  // namespace ugh::data
