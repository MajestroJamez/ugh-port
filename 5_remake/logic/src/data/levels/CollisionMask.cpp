#include "data/levels/CollisionMask.hpp"

namespace ugh::data::levels {

bool CollisionMask::solid(int x, int y) const {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return false;
    return (bits[y * (WIDTH / 8) + x / 8] & (0x80 >> (x % 8))) != 0;
}

}  // namespace ugh::data::levels
