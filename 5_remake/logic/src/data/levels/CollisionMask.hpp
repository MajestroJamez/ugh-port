// The solid pixels of a level.
#pragma once

#include <cstdint>
#include <vector>

#include "data/levels/ScreenSize.hpp"

namespace ugh::data::levels {

/** The solid pixels of a level's background: the whole screen (ScreenSize), nothing outside is solid. */
struct CollisionMask {
    static constexpr int WIDTH = ScreenSize::WIDTH, HEIGHT = ScreenSize::HEIGHT;

    std::vector<uint8_t> bits;   // rows of WIDTH / 8 bytes, the leftmost pixel in the highest bit

    bool solid(int x, int y) const;
};

}  // namespace ugh::data::levels
