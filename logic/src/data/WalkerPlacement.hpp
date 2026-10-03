// A walker in the definition of a level.
#pragma once

#include "data/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data {

/** A walker on its pad, at x, y, walking at `speed` (Fixed per frame; negative: to the left). */
class WalkerPlacement : public EnemyPlacement {
public:
    WalkerPlacement(int pad, units::Fixed x, units::Fixed y, units::Fixed speed) : pad_(pad), x_(x), y_(y), speed_(speed) {}

    int pad() const { return pad_; }
    units::Fixed x() const { return x_; }
    units::Fixed y() const { return y_; }
    units::Fixed speed() const { return speed_; }

    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

private:
    int pad_;
    units::Fixed x_, y_, speed_;
};

}  // namespace ugh::data
