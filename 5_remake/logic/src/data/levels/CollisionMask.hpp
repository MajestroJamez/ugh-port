// The solid pixels of a level.
#pragma once

#include <cstdint>
#include <vector>

namespace ugh::data::levels {

/** The solid pixels of a level's background: 320 x 192, nothing outside is solid. */
struct CollisionMask {
    static constexpr int WIDTH = 320, HEIGHT = 192;

    std::vector<uint8_t> bits;   // rows of WIDTH / 8 bytes, the leftmost pixel in the highest bit

    bool solid(int x, int y) const;
};

}  // namespace ugh::data::levels
