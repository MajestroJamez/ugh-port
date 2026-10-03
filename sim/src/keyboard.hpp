// The keyboard of the original (113b:4567, Host.kt keyboardInterrupt): scancodes from the keyboard interrupt matched
// against the key table, including the 0xE0-prefixed pairs of the extended keys, turned into the held keys of the
// players. Both players play on the keyboard.
#pragma once

#include <cstdint>
#include <vector>

#include "data.hpp"
#include "world.hpp"

namespace ugh {

class Keyboard {
public:
    explicit Keyboard(const std::vector<KeyBinding>& bindings) : bindings_(&bindings), failed_(bindings.size()) {}

    /** A scancode from the keyboard interrupt. */
    void deliver(uint8_t scancode, std::array<Copter, 2>& copters);

    /**
     * 113b:44f1 - Frame.kt readScancode: the last scancode the game loop looks at (Esc, P) and whether it changed
     * since the previous look.
     */
    struct Reading {
        uint8_t scancode;
        bool changed;
    };
    Reading read();

    /** Back to the program start: nothing pressed, no sequence started. */
    void reset();

private:
    const std::vector<KeyBinding>* bindings_;
    std::vector<bool> failed_;   // a binding that does not match the sequence so far (281c + 5)
    int position_ = 0;           // scancodes of the current sequence so far (2647)
    uint8_t lastScancode_ = 0;   // CS:4509
    uint8_t lastRead_ = 0;       // 2643
};

}  // namespace ugh
