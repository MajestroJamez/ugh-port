#include "data/CollisionMask.hpp"

namespace ugh::data {

bool CollisionMask::solid(int index) const {
    if (index < 0 || index >= WIDTH * HEIGHT || bits_.empty()) return false;
    int x = index % WIDTH, y = index / WIDTH;
    return (bits_[y * (WIDTH / 8) + x / 8] & (0x80 >> (x & 7))) != 0;
}

}  // namespace ugh::data
