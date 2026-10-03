#include "data/CollisionMask.hpp"

namespace ugh::data {

bool CollisionMask::solid(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
    return (bits_[y * (WIDTH / 8) + x / 8] & (0x80 >> (x % 8))) != 0;
}

}  // namespace ugh::data
