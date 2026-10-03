// A kind of bonus item.
#pragma once

#include <string>

#include "data/BonusEffect.hpp"
#include "units/Int16.hpp"

namespace ugh::data {

/** A kind of bonus item (a fruit of a tree, the multiplier a quick delivery drops). */
struct BonusKind {
    std::string name;
    BonusEffect effect = BonusEffect::Energy;
    units::Int16 amount;          // energy or lives
    units::Int16 lift;            // its upward speed when dropped, in 1/32 px per frame
    int sprite = 0;
    units::Int16 anchorX, anchorY;   // from its top left corner to its middle and its bottom, in pixels
};

}  // namespace ugh::data
