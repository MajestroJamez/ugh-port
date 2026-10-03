// A flyer in the definition of a level.
#pragma once

#include "data/EnemyPlacement.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** A flyer: it waits `startDelay` frames, then flies across the screen at `speed`. */
class FlyerPlacement : public EnemyPlacement {
public:
    FlyerPlacement(units::Int16 startDelay, units::Fixed speed) : startDelay_(startDelay), speed_(speed) {}

    units::Int16 startDelay() const { return startDelay_; }
    /** Fixed per frame. */
    units::Fixed speed() const { return speed_; }

    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

private:
    units::Int16 startDelay_;
    units::Fixed speed_;
};

}  // namespace ugh::data
