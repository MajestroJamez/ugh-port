#include "world/Water.hpp"

namespace ugh::world {

void Water::fill(units::Fixed level) {
    level_ = level;
    resting_ = false;
    evenFrame_ = 0;
    surfaceFrame_ = 0;
    surfaceDelay_ = 0;
}

void Water::move(units::Fixed speed) {
    if (resting_) {
        resting_ = false;
        return;
    }
    int before = row();
    evenFrame_ ^= 1;
    if (evenFrame_ == 0) {
        level_ += speed;
        if (level_ < units::Fixed()) level_ = units::Fixed();
    }
    if (row() != before) resting_ = true;
}

void Water::animateSurface() {
    if (--surfaceDelay_ >= 0) return;
    surfaceDelay_ = SURFACE_DELAY;
    if (--surfaceFrame_ < 0) surfaceFrame_ = SURFACE_LAST_FRAME;
}

}  // namespace ugh::world
