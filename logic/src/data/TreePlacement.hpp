// A tree in the definition of a level.
#pragma once

#include <utility>
#include <vector>

#include "data/BonusKind.hpp"
#include "data/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** A tree at x, y with the bonus items it drops, one for each passenger that bounces off it. */
class TreePlacement : public EnemyPlacement {
public:
    TreePlacement(units::Fixed x, units::Fixed y, std::vector<const BonusKind*> drops)
        : x_(x), y_(y), drops_(std::move(drops)) {}

    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    const std::vector<const BonusKind*>& drops() const { return drops_; }

    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

private:
    units::Fixed x_, y_;
    std::vector<const BonusKind*> drops_;
};

}  // namespace ugh::data
