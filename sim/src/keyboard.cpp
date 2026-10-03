#include "keyboard.hpp"

#include <algorithm>

namespace ugh {

namespace {

bool& held(Controls& c, KeyBinding::Key key) {
    switch (key) {
        case KeyBinding::Key::Up: return c.up;
        case KeyBinding::Key::Down: return c.down;
        case KeyBinding::Key::Left: return c.left;
        case KeyBinding::Key::Right: return c.right;
        default: return c.fire;
    }
}

}  // namespace

void Keyboard::deliver(uint8_t scancode, std::array<Copter, 2>& copters) {
    lastScancode_ = scancode;
    bool partial = false, matched = false;
    for (size_t i = 0; i < bindings_->size(); i++) {
        if (failed_[i]) continue;
        const KeyBinding& k = (*bindings_)[i];
        // a sequence has at most two scancodes; the one after the last is 0xff
        uint8_t expected = position_ < 2 ? k.scancodes[position_] : 0xff;
        uint8_t following = position_ + 1 < 2 ? k.scancodes[position_ + 1] : 0xff;
        if (scancode != expected) {
            failed_[i] = true;
        } else if (following != 0xff) {
            partial = true;
        } else {
            if (k.key != KeyBinding::Key::None) held(copters[k.player].keys, k.key) = k.press;
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

}  // namespace ugh
