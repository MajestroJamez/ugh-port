// A key of the PC keyboard.
#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "data/PlayerKey.hpp"

namespace ugh::data {

/** A key of the PC keyboard: its scancodes (an extended key has two) and what it does. */
struct KeyBinding {
    /** A player's key pressed or released. */
    struct Action {
        int player = 0;
        PlayerKey key = PlayerKey::Up;
        bool press = false;
    };

    std::vector<uint8_t> scancodes;
    std::optional<Action> action;   // none: the scancodes are swallowed (the fake shifts of extended keys)
};

}  // namespace ugh::data
