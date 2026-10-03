// A tree in the definition of a level.
#pragma once

#include <utility>
#include <vector>

#include "data/kinds/BonusKind.hpp"
#include "data/levels/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data::levels {

/** A tree at x, y with the bonus items it drops, one for each passenger that bounces off it. */
struct TreePlacement : EnemyPlacement {
    TreePlacement(units::Fixed atX, units::Fixed atY, std::vector<const kinds::BonusKind*> itsDrops)
        : x(atX), y(atY), drops(std::move(itsDrops)) {}
    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

    units::Fixed x, y;
    std::vector<const kinds::BonusKind*> drops;
};

}  // namespace ugh::data::levels
