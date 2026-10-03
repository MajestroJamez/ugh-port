// The PC keyboard.
#pragma once

#include <cstdint>
#include <vector>

#include "data/KeyBinding.hpp"

namespace ugh::input {

/**
 * The PC keyboard (Adapter): scancodes from the keyboard, matched against the key bindings of the data (extended keys
 * come as two scancodes), become the keys the players hold; the game loop also looks at the last scancode (a caption
 * waits for it to change, Esc gives up, P would pause).
 */
class PcKeyboard {
public:
    /** The last scancode, and whether it changed since the last look. */
    struct Reading {
        uint8_t scancode = 0;
        bool changed = false;
    };

    explicit PcKeyboard(const std::vector<data::KeyBinding>& bindings);

    /** A scancode from the keyboard; the action of a completed key binding, if any. */
    const data::KeyBinding::Action* deliver(uint8_t scancode);

    /** The game loop looks at the last scancode. */
    Reading readLastScancode();

private:
    static constexpr int LONGEST_SEQUENCE = 2;

    const std::vector<data::KeyBinding>* bindings_;
    std::vector<bool> failed_;   // the bindings that do not match the sequence so far
    int position_ = 0;           // scancodes of the current sequence so far
    uint8_t lastScancode_ = 0;
    uint8_t lastRead_ = 0;
};

}  // namespace ugh::input
