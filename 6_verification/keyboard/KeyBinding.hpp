// A key of the PC keyboard.
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "data/PlayerKey.hpp"

namespace ugh::keyboard {

/**
 * A key of the PC keyboard (the `key` records of the game data): its scancodes (an extended key has two) and what it
 * does.
 */
struct KeyBinding {
    /** A pilot's key pressed or released. */
    struct Action {
        int player = 0;
        data::PlayerKey key = data::PlayerKey::Up;
        bool press = false;
    };

    std::vector<uint8_t> scancodes;
    std::optional<Action> action;   // none: the scancodes are swallowed (the fake shifts of extended keys)
};

}  // namespace ugh::keyboard
