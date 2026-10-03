#include "model/Water.hpp"

namespace ugh::model {

namespace {

constexpr uint8_t SURFACE_DELAY = 6;    // frames per frame of the surface animation
constexpr core::Word SURFACE_LAST = 2;  // the animation runs 2, 1, 0

}  // namespace

void Water::startAttempt() {
    s_.hold = 0;
    s_.toggle = 0;
    s_.surfaceFrame = 0;
    s_.surfaceDelay = 0;
}

void Water::fillTo(core::Fixed level) {
    s_.level = level;
    s_.row = level.pixels();
}

void Water::move(core::Word speed) {
    if (s_.hold != 0) {   // the frame after the row changed only redraws
        s_.hold = 0;
        return;
    }
    core::Word row = s_.row;
    s_.toggle ^= 1;
    if (s_.toggle == 0) {
        s_.level += core::Fixed(speed);
        if (s_.level < core::Fixed(0)) s_.level = core::Fixed(0);
    }
    s_.row = s_.level.pixels();
    if (s_.row != row) s_.hold--;
}

void Water::animateSurface() {
    s_.surfaceDelay--;
    if (s_.surfaceDelay & 0x80) {   // DEC went below zero
        s_.surfaceDelay = SURFACE_DELAY;
        --s_.surfaceFrame;
        if (s_.surfaceFrame < 0) s_.surfaceFrame = SURFACE_LAST;
    }
}

}  // namespace ugh::model
