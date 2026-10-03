// A tree in the definition of a level.
#pragma once

#include <utility>
#include <vector>

#include "data/BonusKind.hpp"
#include "data/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** A tree at x, y with the bonus items it drops, one for each passenger that bounces off it. */
struct TreePlacement : EnemyPlacement {
    TreePlacement(units::Fixed atX, units::Fixed atY, std::vector<const BonusKind*> itsDrops)
        : x(atX), y(atY), drops(std::move(itsDrops)) {}
    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

    units::Fixed x, y;
    std::vector<const BonusKind*> drops;
};

}  // namespace ugh::data
