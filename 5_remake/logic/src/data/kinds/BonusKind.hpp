// A kind of bonus item.
#pragma once

#include <string>

#include "data/kinds/BonusEffect.hpp"
#include "units/Fixed.hpp"

namespace ugh::data::kinds {

/** A kind of bonus item (a fruit of a tree, the multiplier a quick delivery drops). */
struct BonusKind {
    std::string name;
    BonusEffect effect = BonusEffect::Energy;
    int amount = 0;      // energy or lives
    units::Fixed lift;   // its upward speed when dropped, per frame
    int sprite = 0;
    int anchorX = 0, anchorY = 0;   // from its top left corner to its middle and its bottom, in pixels
};

}  // namespace ugh::data::kinds
