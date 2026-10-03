#include "input/Keyboard.hpp"

#include <algorithm>

namespace ugh::input {

namespace {

constexpr int LONGEST_SEQUENCE = 2;   // scancodes

}  // namespace

Keyboard::Keyboard(const std::vector<data::KeyBinding>& bindings) : bindings_(&bindings), failed_(bindings.size()) {}

void Keyboard::deliver(uint8_t scancode, model::Level& level) {
    lastScancode_ = scancode;
    bool partial = false, matched = false;
    for (size_t i = 0; i < bindings_->size(); i++) {
        if (failed_[i]) continue;
        const data::KeyBinding& k = (*bindings_)[i];
        // the scancode after the last one of a sequence is NO_SCANCODE
        uint8_t expected = position_ < LONGEST_SEQUENCE ? k.scancodes[position_] : data::KeyBinding::NO_SCANCODE;
        uint8_t following = position_ + 1 < LONGEST_SEQUENCE ? k.scancodes[position_ + 1] : data::KeyBinding::NO_SCANCODE;
        if (scancode != expected) {
            failed_[i] = true;
        } else if (following != data::KeyBinding::NO_SCANCODE) {
            partial = true;
        } else {
            level.copter(k.player).controls().set(k.key, k.press);
            matched = true;
            break;
        }
    }
    if (matched || !partial) {
        position_ = 0;
        std::fill(failed_.begin(), failed_.end(), false);
    } else {
        position_++;
    }
}

Keyboard::Reading Keyboard::read() {
    uint8_t previous = lastRead_;
    lastRead_ = lastScancode_;
    return {lastRead_, lastRead_ != previous};
}

void Keyboard::reset() {
    std::fill(failed_.begin(), failed_.end(), false);
    position_ = 0;
    lastScancode_ = lastRead_ = 0;
}

}  // namespace ugh::input
