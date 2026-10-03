// A blower in the definition of a level.
#pragma once

#include "data/levels/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data::levels {

/** A blower at x, y. */
struct BlowerPlacement : EnemyPlacement {
    BlowerPlacement(units::Fixed atX, units::Fixed atY) : x(atX), y(atY) {}
    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

    units::Fixed x, y;
};

}  // namespace ugh::data::levels
