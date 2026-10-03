// A walker in the definition of a level.
#pragma once

#include "data/levels/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data::levels {

/** A walker on its pad, at x, y, walking at `speed` (Fixed per frame; negative: to the left). */
struct WalkerPlacement : EnemyPlacement {
    WalkerPlacement(int onPad, units::Fixed atX, units::Fixed atY, units::Fixed perFrame)
        : pad(onPad), x(atX), y(atY), speed(perFrame) {}
    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

    int pad;
    units::Fixed x, y, speed;
};

}  // namespace ugh::data::levels
