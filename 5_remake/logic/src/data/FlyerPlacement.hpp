// A flyer in the definition of a level.
#pragma once

#include "data/EnemyPlacement.hpp"
#include "units/Fixed.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** A flyer: it waits `startDelay` frames, then flies across the screen at `speed`. */
struct FlyerPlacement : EnemyPlacement {
    FlyerPlacement(units::Int16 delay, units::Fixed perFrame) : startDelay(delay), speed(perFrame) {}
    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

    units::Int16 startDelay;
    units::Fixed speed;   // per frame
};

}  // namespace ugh::data
