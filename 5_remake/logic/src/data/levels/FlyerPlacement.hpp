// A flyer in the definition of a level.
#pragma once

#include "data/levels/EnemyPlacement.hpp"
#include "units/Fixed.hpp"

namespace ugh::data::levels {

/** A flyer: it waits `startDelay` frames, then flies across the screen at `speed`. */
struct FlyerPlacement : EnemyPlacement {
    FlyerPlacement(int delay, units::Fixed perFrame) : startDelay(delay), speed(perFrame) {}
    void accept(EnemyPlacementVisitor& visitor) const override { visitor.visit(*this); }

    int startDelay = 0;
    units::Fixed speed;   // per frame
};

}  // namespace ugh::data::levels
