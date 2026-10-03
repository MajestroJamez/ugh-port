// Where a sprite stands and how big it is.
#pragma once

#include "core/Word.hpp"

namespace ugh::data {

/**
 * The anchor and the touch box of a sprite (descriptor +0 .. +6), in pixels: x / y lead from the sprite's top left
 * to the point that stands on a pad (x the middle, y the feet); halfWidth / halfHeight are half the size of the box
 * a copter touches.
 */
struct Box {
    core::Word x, y, halfWidth, halfHeight;
};

}  // namespace ugh::data
