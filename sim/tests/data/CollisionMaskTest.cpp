#include <cstdint>
#include <vector>

#include "TestFramework.hpp"
#include "data/CollisionMask.hpp"

using ugh::data::CollisionMask;

TEST(collisionMaskIsALinearPage) {
    std::vector<uint8_t> bits(CollisionMask::WIDTH / 8 * CollisionMask::HEIGHT);
    bits[4 * 48 + 47] = 0x01;   // pixel 383 of row 4
    CollisionMask mask(bits);
    CHECK(mask.solid(4 * 384 + 383));
    CHECK(mask.solid(5 * 384 - 1));      // a probe running over the right edge reads the next row
    CHECK(!mask.solid(4 * 384 + 382));
    CHECK(!mask.solid(-1));              // outside the page nothing is solid
    CHECK(!mask.solid(384 * 192));
}
