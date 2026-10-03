// A blower in the definition of a level.
#pragma once

#include "data/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** A blower at x, y. */
class BlowerPlacement : public EnemyPlacement {
public:
    BlowerPlacement(units::Fixed x, units::Fixed y) : x_(x), y_(y) {}

    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }

    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

private:
    units::Fixed x_, y_;
};

}  // namespace ugh::data
