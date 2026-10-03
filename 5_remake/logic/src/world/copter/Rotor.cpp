#include "world/copter/Rotor.hpp"

namespace ugh::world::copter {

namespace {

constexpr int MAX_EFFORT = 90;   // the rotor spins no faster than with this effort
constexpr int PERIOD = 200;      // effort per rotor sprite

}  // namespace

void Rotor::spin(int firstSprite, int lastSprite) {
    counter_ -= effort_ < MAX_EFFORT ? effort_ : MAX_EFFORT;
    if (counter_ >= 0) return;
    counter_ += PERIOD;
    sprite_ = sprite_ + 1 > lastSprite ? firstSprite : sprite_ + 1;
}

}  // namespace ugh::world::copter
