// A kind of bonus item.
#pragma once

#include <cstdint>

#include "core/Word.hpp"
#include "data/Sprite.hpp"

namespace ugh::data {

/** A kind of bonus item (descriptor at 7970, 7984 ... and 7a38, 0x14 bytes each). */
struct BonusKind {
    /** What collecting it gives (the values are the event values of the C API). */
    enum class Effect { Energy = 0, Life = 1, Multiplier = 2 };

    uint16_t origin = 0;     // the offset in the original's data (its identity in the replays)
    core::Word x, y;         // +0 / +2: from the top left to the middle / the bottom (lands on pads), in pixels
    core::Word lift;         // +0c: upward speed when dropped (1/32 px per frame)
    core::Word amount;       // +0e: energy or lives
    Sprite sprite = 0;       // +10
    Effect effect = Effect::Energy;   // +12
};

}  // namespace ugh::data
