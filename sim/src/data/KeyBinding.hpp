// A key of the keyboard table.
#pragma once

#include <array>
#include <cstdint>

namespace ugh::data {

/** A key of the keyboard table (DGROUP:281c): a scancode or an 0xE0-prefixed pair, and what it does. */
struct KeyBinding {
    enum class Key { Up, Down, Left, Right, Fire, None };

    /** The second scancode of a single-scancode key. */
    static constexpr uint8_t NO_SCANCODE = 0xff;

    std::array<uint8_t, 2> scancodes{};
    int player = 0;
    Key key = Key::None;                  // None: swallowed (the fake shifts of extended keys)
    bool press = false;                   // press or release
};

}  // namespace ugh::data
