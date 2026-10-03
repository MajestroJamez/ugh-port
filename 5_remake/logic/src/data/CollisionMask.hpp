// The solid pixels of a level.
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace ugh::data {

/** The solid pixels of a level's background: 320 x 192, nothing outside is solid. */
class CollisionMask {
public:
    static constexpr int WIDTH = 320, HEIGHT = 192;

    CollisionMask() = default;
    /** From rows of WIDTH / 8 bytes, the leftmost pixel in the highest bit. */
    explicit CollisionMask(std::vector<uint8_t> bits) : bits_(std::move(bits)) {}

    bool solid(int x, int y) const;

private:
    std::vector<uint8_t> bits_;
};

}  // namespace ugh::data
