// Where a sprite stands and how big its touch box is.
#pragma once

namespace ugh::data {

/**
 * The anchor and the touch box of a sprite, in pixels: x, y lead from the sprite's top left corner to the point that
 * stands on a pad (x the middle, y the feet); halfWidth and halfHeight are half the size of the box a copter touches.
 */
struct Box {
    int x = 0, y = 0, halfWidth = 0, halfHeight = 0;
};

}  // namespace ugh::data
